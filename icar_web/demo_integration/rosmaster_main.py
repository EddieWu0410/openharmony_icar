import socket
import os
import time
import threading
import cv2 as cv
import sys
import numpy as np
import struct
import datetime
import license
import signal

import subprocess
import time

from flask import Flask, render_template, Response
from gevent import pywsgi

from Rosmaster_Lib import Rosmaster
from camera_rosmaster import Rosmaster_Camera
from wifi_rosmaster import Rosmaster_WIFI


# ---------------------------------------------------------------------------------
# 主应用类
# ---------------------------------------------------------------------------------
class MyRosmasterApp:
    def __init__(self, debug=False):
        # --- 基础配置 ---
        self.g_debug = debug
        print(f"Debug mode is {'On' if self.g_debug else 'Off'}")

        # --- 初始化核心库 ---
        self.g_bot = Rosmaster(debug=self.g_debug)
        self.g_bot.create_receive_threading()  # 启动串口数据接收线程
        self.g_wifi = Rosmaster_WIFI(self.g_bot, debug=self.g_debug)

        # --- 摄像头优化：采用生产者-消费者模型 ---
        self.g_camera = Rosmaster_Camera(video_id=0, debug=self.g_debug)
        self.latest_frame = None
        self.frame_lock = threading.Lock()
        self.camera_thread = threading.Thread(target=self._update_camera_frame, daemon=True)
        self.camera_thread.start()
        if self.g_debug:
            print("--- Camera reading thread started (Producer) ---")
            
        # --- 视频流处理服务器相关 ---
        self.video_server_enabled = False
        self.video_server_host = '0.0.0.0'
        self.video_server_port = 12345
        self.video_server_socket = None
        self.video_server_thread = None
        self.processed_frame = None 

        # --- 状态变量 ---
        self.g_init = False
        self.g_mode = 'Home'
        self.g_car_type = 1  # 默认小车类型
        self.g_car_stabilize_state = 0
        self.g_tcp_ip = "x.x.x.x"
        self.g_socket = None

        # --- 运动控制 ---
        self.g_speed_ctrl_xy = 100
        self.g_speed_ctrl_z = 100
        self.g_motor_speed = [0, 0, 0, 0]

        # --- 拍照与录像 ---
        self.g_capture_image = False
        self.g_capture_video = False
        self.g_video_writer = None
        self.g_video_file_name = None
        
        # --- 雷达避障状态 ---
        self.g_laser_avoidance = False
        # --- 雷达警卫 ---
        self.g_laser_warning = False
        # --- 雷达警卫 ---
        self.g_laser_tracker = False
        # --- 雷达功能共用 ---
        self.g_laser_container_id = None  # 存储容器ID
        self.g_laser_processes = []  # 存储雷达相关进程

        def set_debug(self, debug=True):
            self.g_debug = debug

    # --------------------------------------------------------------------------
    # 摄像头后台读取线程 (生产者)
    # --------------------------------------------------------------------------
    def _update_camera_frame(self):
        """
        在后台持续运行，不断从摄像头获取最新帧并存入共享变量。
        """
        while True:
            success, frame = self.g_camera.get_frame()
            
            if success:
                # print("摄像头采集帧均值：", np.mean(frame))
                with self.frame_lock:
                    self.latest_frame = frame
            else:
                # 如果获取失败，等待一会再重试，避免CPU空转
                if self.g_debug:
                    print("--- Camera frame grab failed, retrying... ---")
                self.g_camera.reconnect()
                time.sleep(0.5)

    # --------------------------------------------------------------------------
    # 视频流处理 (消费者)
    # --------------------------------------------------------------------------
    def mode_handle(self):
        """
        一个生成器函数，用于Flask视频流。它从共享变量中获取图像。
        """
        if self.g_debug:
            print("--- Video stream handler started (Consumer) ---")
        
        t_start = time.time()
        m_fps = 0
        while True:
            frame_to_send = None
            with self.frame_lock:
                if self.latest_frame is not None:
                    # 复制一份以在解锁后安全使用
                    frame_to_send = self.latest_frame.copy()

            if frame_to_send is None:
                time.sleep(0.05)  # 等待有效帧
                continue

            # --- 功能叠加: 拍照和录像 ---
            self.capture_img(frame_to_send)
            self.capture_video(frame_to_send)
            
            # --- 功能叠加: 显示FPS ---
            m_fps += 1
            elapsed_time = time.time() - t_start
            if elapsed_time > 1: # 每秒更新一次FPS
                fps = m_fps / elapsed_time
                text = f"FPS: {int(fps)}"
                cv.putText(frame_to_send, text, (10, 25), cv.FONT_HERSHEY_TRIPLEX, 0.7, (0, 255, 0), 1)
                t_start = time.time()
                m_fps = 0

            # --- 编码并发送 ---
            ret, img_encode = cv.imencode('.jpg', frame_to_send)
            if ret:
                yield (b'--frame\r\n'
                    b'Content-Type: image/jpeg\r\n\r\n' + img_encode.tobytes() + b'\r\n')
            else:
                time.sleep(0.01)
                
    # --------------------------------------------------------------------------
    # 视频流发送到服务器处理函数
    # --------------------------------------------------------------------------
    def _send_frames_to_server(self):
        """
        独立线程：从共享变量中获取图像帧并发送到视频处理服务器，同时接收处理后的帧并保存到单独变量
        """
        print("启动")
        while self.video_server_enabled:
            print("发送")
            frame_to_send = None
            with self.frame_lock:
                if self.latest_frame is not None:
                    # 复制一份以在解锁后安全使用
                    frame_to_send = self.latest_frame.copy()
                    if frame_to_send is not None:
                        print("客户端即将发送帧，均值：", np.mean(frame_to_send))

            if frame_to_send is None:
                time.sleep(0.05)  # 等待有效帧
                continue

            try:
                # 发送图像到服务器
                ret, img_encode = cv.imencode('.jpg', frame_to_send, [cv.IMWRITE_JPEG_QUALITY, 90])
                if not ret:
                    
                    print("--- Failed to encode frame ---")
                    time.sleep(0.01)
                    continue

                # 发送图像数据大小和数据 - 修改为与服务端匹配的格式
                img_data = img_encode.tobytes()
                img_size = len(img_data)
                
                # 发送数据大小（4字节，big-endian格式）
                size_data = struct.pack('!I', img_size)
                self.video_server_socket.sendall(size_data)
                
                # 发送图像数据
                self.video_server_socket.sendall(img_data)
                
                if self.g_debug:
                    print(f"--- Sent frame to server, size: {img_size} bytes ---")

                # 接收服务器处理后的帧
                processed_frame = self._receive_frame_from_server()
                if processed_frame is not None:
                    # 保存处理后的帧到单独变量
                    with self.frame_lock:
                        self.processed_frame = processed_frame
                    
                    if self.g_debug:
                        print("--- Saved processed frame to separate variable ---")
                
                # 接收检测结果消息（如果有的话）
                try:
                    # 设置非阻塞接收消息
                    self.video_server_socket.settimeout(0.01)  # 10ms超时
                    msg = self.video_server_socket.recv(1024).decode('utf-8')
                    if msg and msg != "no_detection":
                        if self.g_debug:
                            print(f"--- Detection result: {msg} ---")
                except socket.timeout:
                    pass  # 没有消息时忽略
                except Exception as e:
                    print(f"--- Error receiving detection message: {e} ---")
                finally:
                    self.video_server_socket.settimeout(None)  # 恢复阻塞模式

            except Exception as e:
                print(f"--- Error communicating with server: {e} ---")
                self._disconnect_video_server()
                break

            time.sleep(0.033)  # 控制发送频率约30fps
            
        print("退出",self.video_server_enabled)
            
    def yolo_mode_handle(self):
        """
        专门用于YOLO处理后视频流的生成器函数，从单独的processed_frame变量中获取帧
        """
        
        print("--- YOLO Video stream handler started ---")
        
        while True:
            frame_to_send = None
            
            # 从单独的processed_frame变量获取YOLO处理后的帧
            with self.frame_lock:
                if hasattr(self, 'processed_frame') and self.processed_frame is not None:
                    frame_to_send = self.processed_frame.copy()
            
            if frame_to_send is None:
                # 如果没有YOLO处理帧，返回提示图像
                frame_to_send = np.zeros((480, 640, 3), dtype=np.uint8)
                cv.putText(frame_to_send, "YOLO Server Not Connected", 
                    (120, 200), cv.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                cv.putText(frame_to_send, "or No Detection Data", 
                    (150, 280), cv.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
                time.sleep(0.1)  # 减少CPU使用

            # 编码并发送
            ret, img_encode = cv.imencode('.jpg', frame_to_send)
            if ret:
                yield (b'--frame\r\n'
                    b'Content-Type: image/jpeg\r\n\r\n' + img_encode.tobytes() + b'\r\n')
            else:
                time.sleep(0.01)
                
    def get_yolo_stream_status(self):
        """
        获取YOLO视频流的详细状态信息
        """
        status = {
            'enabled': self.video_server_enabled,
            'connected': self.video_server_socket is not None,
            'server': f"{self.video_server_host}:{self.video_server_port}",
            'has_processed_frame': hasattr(self, 'processed_frame') and self.processed_frame is not None
        }
        return status
            
    def _receive_frame_from_server(self):
        """
        从服务器接收处理后的视频帧
        """
        try:
            # 接收帧大小（4字节）
            size_data = b''
            self.video_server_socket.settimeout(0.5)  # 设置超时时间，避免阻塞
            while len(size_data) < 4:
                try:
                    chunk = self.video_server_socket.recv(4 - len(size_data))
                    if not chunk:
                        self.video_server_socket.settimeout(None)
                        return None
                    size_data += chunk
                except socket.timeout:
                    # 超时未收到数据，跳出循环
                    self.video_server_socket.settimeout(None)
                    if self.g_debug:
                        print("--- Timeout waiting for frame size header ---")
                    return None
            self.video_server_socket.settimeout(None)  # 恢复阻塞模式

            frame_size = struct.unpack('!I', size_data)[0]

            # 接收完整的帧数据
            frame_data = b''
            self.video_server_socket.settimeout(0.5)
            while len(frame_data) < frame_size:
                try:
                    chunk = self.video_server_socket.recv(min(frame_size - len(frame_data), 4096))
                    if not chunk:
                        break
                    frame_data += chunk
                except socket.timeout:
                    break
            self.video_server_socket.settimeout(None)
            if len(frame_data) < frame_size:
                if self.g_debug:
                    print(f"--- Incomplete frame received: {len(frame_data)}/{frame_size} bytes ---")
                return None

            # 解码图像
            nparr = np.frombuffer(frame_data, np.uint8)
            frame = cv.imdecode(nparr, cv.IMREAD_COLOR)
            return frame

        except Exception as e:
            if self.g_debug:
                print(f"--- Error receiving frame from server: {e} ---")
            return None
        
    def _connect_video_server(self):
        """
        连接到视频处理服务器
        """
        try:
            self.video_server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.video_server_socket.connect((self.video_server_host, self.video_server_port))
            print(f"--- Connected to video server: {self.video_server_host}:{self.video_server_port} ---")
            return True
        except Exception as e:
            if self.g_debug:
                print(f"--- Failed to connect to video server: {e} ---")
            self.video_server_socket = None
            return False

    def start_video_server_stream(self, host='192.168.43.253', port=12345):
        """
        启动视频流发送到服务器的功能
        """
        if self.video_server_enabled:
            if self.g_debug:
                print("--- Video server stream already running ---")
            return True

        self.video_server_host = host
        self.video_server_port = port
        
        # 连接到服务器
        if not self._connect_video_server():
            return False

        self.video_server_enabled = True
        
        # 启动发送线程
        self.video_server_thread = threading.Thread(target=self._send_frames_to_server, daemon=False)
        self.video_server_thread.start()
        
        if self.g_debug:
            print("--- Video server stream started ---")
        return True

    def stop_video_server_stream(self):
        """
        停止视频流发送到服务器的功能，并清理相关资源
        """
        if not self.video_server_enabled:
            if self.g_debug:
                print("--- Video server stream not running ---")
            return

        self.video_server_enabled = False
        
        # 清理处理后的帧
        with self.frame_lock:
            self.processed_frame = None
        
        # 关闭OpenCV窗口
        try:
            cv.destroyWindow('YOLO Detection Results')
        except:
            pass
        
        # 等待线程结束
        if self.video_server_thread and self.video_server_thread.is_alive():
            self.video_server_thread.join(timeout=2.0)
        
        # 断开连接
        self._disconnect_video_server()
        
        if self.g_debug:
            print("--- Video server stream stopped and resources cleaned ---")

    # --------------------------------------------------------------------------
    # 拍照与录像相关方法
    # --------------------------------------------------------------------------
    def capture_img(self, frame):
        if self.g_capture_image:
            dt = datetime.datetime.now().strftime('%Y%m%d%H%M%S')
            img_dir = './capture/'
            if not os.path.exists(img_dir):
                os.makedirs(img_dir)
            
            file_name = f"{img_dir}{dt}.jpg"
            cv.imwrite(file_name, frame)
            print(f"Image saved: {file_name}")
            self.g_capture_image = False # 自动复位

    def capture_video(self, frame):
        if self.g_capture_video:
            if self.g_video_writer is None:
                # 开始录制
                dt = datetime.datetime.now().strftime('%Y%m%d%H%M%S')
                video_dir = './capture/'
                if not os.path.exists(video_dir):
                    os.makedirs(video_dir)
                
                self.g_video_file_name = f"{video_dir}{dt}.avi"
                fourcc = cv.VideoWriter_fourcc(*'XVID')
                height, width, _ = frame.shape
                self.g_video_writer = cv.VideoWriter(self.g_video_file_name, fourcc, 20.0, (width, height))
                print(f"Video recording started: {self.g_video_file_name}")
            
            self.g_video_writer.write(frame)
        else:
            if self.g_video_writer is not None:
                # 结束录制
                self.g_video_writer.release()
                print(f"Video saved: {self.g_video_file_name}")
                self.g_video_writer = None
                self.g_video_file_name = None

    # --------------------------------------------------------------------------
    # 数据返回给APP的接口
    # --------------------------------------------------------------------------
    def _send_tcp_data(self, tcp_socket, data):
        if tcp_socket:
            try:
                tcp_socket.send(data.encode(encoding="utf-8"))
                if self.g_debug:
                    print(f"TCP TX: {data}")
            except Exception as e:
                if self.g_debug:
                    print(f"TCP send error: {e}")

    def return_bot_version(self, tcp):
        T_CARTYPE = self.g_car_type
        T_FUNC = 0x01
        T_LEN = 0x04
        version = int(self.g_bot.get_version() * 10)
        if version < 0: version = 0
        checknum = (T_CARTYPE + T_FUNC + T_LEN + version) % 256
        data = f"$%02x%02x%02x%02x%02x#" % (T_CARTYPE, T_FUNC, T_LEN, version, checknum)
        self._send_tcp_data(tcp, data)

    def return_battery_voltage(self, tcp):
        T_CARTYPE = self.g_car_type
        T_FUNC = 0x02
        T_LEN = 0x04
        vol = int(self.g_bot.get_battery_voltage() * 10)
        if vol < 0: vol = 0
        checknum = (T_CARTYPE + T_FUNC + T_LEN + vol) % 256
        data = f"$%02x%02x%02x%02x%02x#" % (T_CARTYPE, T_FUNC, T_LEN, vol, checknum)
        self._send_tcp_data(tcp, data)

    def return_car_speed(self, tcp, speed_xy, speed_z):
        T_CARTYPE = self.g_car_type
        T_FUNC = 0x16
        T_LEN = 0x06
        checknum = (T_CARTYPE + T_FUNC + T_LEN + int(speed_xy) + int(speed_z)) % 256
        data = f"$%02x%02x%02x%02x%02x%02x#" % (T_CARTYPE, T_FUNC, T_LEN, int(speed_xy), int(speed_z), checknum)
        self._send_tcp_data(tcp, data)

    def return_car_stabilize(self, tcp, state):
        T_CARTYPE = self.g_car_type
        T_FUNC = 0x17
        T_LEN = 0x04
        checknum = (T_CARTYPE + T_FUNC + T_LEN + int(state)) % 256
        data = f"$%02x%02x%02x%02x%02x#" % (T_CARTYPE, T_FUNC, T_LEN, int(state), checknum)
        self._send_tcp_data(tcp, data)

    def return_car_current_speed(self, tcp):
        T_CARTYPE = self.g_car_type
        T_FUNC = 0x22
        T_LEN = 0x0E
        speed = self.g_bot.get_motion_data()
        num_x = int(speed[0] * 100)
        num_y = int(speed[1] * 100)
        num_z = int(speed[2] * 20)
        speed_x = num_x.to_bytes(2, byteorder='little', signed=True)
        speed_y = num_y.to_bytes(2, byteorder='little', signed=True)
        speed_z = num_z.to_bytes(2, byteorder='little', signed=True)
        checknum = (T_CARTYPE + T_FUNC + T_LEN + speed_x[0] + speed_x[1] + speed_y[0] + speed_y[1] + speed_z[0] + speed_z[1]) % 256
        data = f"$%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x#" % \
            (T_CARTYPE, T_FUNC, T_LEN, speed_x[0], speed_x[1], speed_y[0], speed_y[1], speed_z[0], speed_z[1], checknum)
        self._send_tcp_data(tcp, data)

    # --------------------------------------------------------------------------
    # 指令解析与执行
    # --------------------------------------------------------------------------
    def parse_data(self, sk_client, data):
        data_size = len(data) 
        # 长度校验
        try:
            declared_len = int(data[5:7], 16)
            if declared_len != data_size - 8:
                if self.g_debug: print(f"Length mismatch! Declared: {declared_len}, Actual: {data_size-8}")
                return
        except ValueError:
            if self.g_debug: print(f"Invalid length field: {data[5:7]}")
            return
            
        # 和校验
        try:
            checknum_calc = sum(int(data[i:i+2], 16) for i in range(1, data_size-3, 2)) % 256
            checknum_recv = int(data[data_size-3:data_size-1], 16)
            if checknum_calc != checknum_recv:
                if self.g_debug: print(f"Checksum mismatch! Calculated: {checknum_calc}, Received: {checknum_recv}")
                return
        except (ValueError, IndexError):
            if self.g_debug: print(f"Invalid checksum or data format.")
            return

        # --- 指令解析开始 ---
        num_carType = int(data[1:3], 16)
        if 0 < num_carType <= 5 and self.g_car_type != num_carType:
            self.g_car_type = num_carType
            self.g_bot.set_car_type(self.g_car_type)
            if self.g_debug: print(f"Car type set to: {self.g_car_type}")
        
        cmd = data[3:5].upper()
        if self.g_debug: print(f"--- CMD RX: {cmd} ---")
        
        # 根据指令执行不同操作
        if cmd == "0F":  # 进入界面
            func = int(data[7:9], 16)
            if self.g_debug: print(f"Enter page function: {func}")
            if func == 1: self.g_mode = 'Standard'
            elif func == 2: self.g_mode = 'MecanumWheel'
            else: self.g_mode = 'Home'
            if self.g_debug: print(f"--- Mode changed to: {self.g_mode} ---")

        elif cmd == "01": self.return_bot_version(sk_client)
        elif cmd == "02": self.return_battery_voltage(sk_client)

        elif cmd == "10": # 摇杆控制
            num_x = int(data[7:9], 16)
            num_y = int(data[9:11], 16)
            if num_x > 127: num_x -= 256
            if num_y > 127: num_y -= 256
            speed_x = num_y / 100.0
            speed_y = -num_x / 100.0
            self.g_bot.set_car_motion(speed_x, speed_y, 0)

        elif cmd == "13": # 蜂鸣器
            state = int(data[7:9], 16)
            delay = int(data[9:11], 16)
            delay_ms = 1 if state > 0 and delay == 255 else delay * 10 if state > 0 else 0
            self.g_bot.set_beep(delay_ms)

        elif cmd == "15": # 按键控制
            direction = int(data[7:9], 16)
            if self.g_mode == 'Standard':
                self.g_bot.set_car_run(direction, self.g_speed_ctrl_xy, self.g_car_stabilize_state)
            else:
                if self.g_debug: print(f"Ignoring move command in '{self.g_mode}' mode.")

        elif cmd == '16': # 速度设置
            self.g_speed_ctrl_xy = np.clip(int(data[7:9], 16), 0, 100)
            self.g_speed_ctrl_z = np.clip(int(data[9:11], 16), 0, 100)
            if self.g_debug: print(f"Speed set to XY:{self.g_speed_ctrl_xy}, Z:{self.g_speed_ctrl_z}")
            
        elif cmd == '17': # 自稳开关
            self.g_car_stabilize_state = 1 if int(data[7:9], 16) > 0 else 0
            if self.g_debug: print(f"Stabilize mode set to: {self.g_car_stabilize_state}")

        elif cmd in ('20', '21'): # 麦克纳姆轮控制
            if cmd == '20': # 单轮
                num_id = int(data[7:9], 16)
                num_speed = int(data[9:11], 16)
                if num_speed > 127: num_speed -= 256
                num_speed = np.clip(num_speed, -100, 100)
                if num_id == 0: self.g_motor_speed = [0, 0, 0, 0]
                else: self.g_motor_speed[num_id-1] = num_speed
            else: # 四轮更新 (cmd == '21')
                speeds = [int(data[i:i+2], 16) for i in range(7, 15, 2)]
                for i in range(4):
                    if speeds[i] > 127: speeds[i] -= 256
                self.g_motor_speed = speeds
            self.g_bot.set_motor(*self.g_motor_speed)

        elif cmd == "30": # 灯带颜色
            led_id, r, g, b = [int(data[i:i+2], 16) for i in range(7, 15, 2)]
            self.g_bot.set_colorful_lamps(led_id, r, g, b)

        elif cmd == "31": # 灯带特效
            effect, speed = [int(data[i:i+2], 16) for i in range(7, 11, 2)]
            self.g_bot.set_colorful_effect(effect, speed, 255)

        elif cmd == '60': self.g_capture_image = True
        
        elif cmd == '61': self.g_capture_video = True
        
        elif cmd == '62': self.g_capture_video = False
        
        elif cmd == '63': self.g_bot.set_follow_line(1)
        
        elif cmd == '64': self.g_bot.set_follow_line(0)
        
        elif cmd == '70': # 开启雷达避障
            if not self.g_laser_avoidance:
                self.g_laser_avoidance = True
                self.laser_avoidance()
                if self.g_debug: print("Laser avoidance started")
                
        elif cmd == '71': # 关闭雷达避障
            if self.g_laser_avoidance:
                self.avoidance_close()  # 调用新的关闭函数
                if self.g_debug: print("Laser avoidance stopped")
                
        elif cmd == '72': # 开启雷达警卫
            if not self.g_laser_warning:
                self.g_laser_warning = True
                self.laser_warning()
                if self.g_debug: print("Laser warning started")
                
        elif cmd == '73': # 关闭雷达警卫
            if self.g_laser_warning:
                self.warning_close()
                if self.g_debug: print("Laser warning stopped")
                
        elif cmd == '74': # 开启雷达追踪
            if not self.g_laser_tracker:
                self.g_laser_tracker = True
                self.laser_tracker()
                if self.g_debug: print("Laser tracker started")
                
        elif cmd == '75': # 关闭雷达追踪
            if self.g_laser_tracker:
                self.tracker_close()
                if self.g_debug: print("Laser tracker stopped")

    # --------------------------------------------------------------------------
    # TCP服务器
    # --------------------------------------------------------------------------
    def start_tcp_server(self, ip, port):
        self.g_init = True
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(('127.0.0.1', port))
        sock.listen(1)
        print(f"TCP Command Server listening on 127.0.0.1:{port}")

        while True:
            print("Waiting for App to connect...")
            client_socket, address = sock.accept()
            self.g_socket = client_socket
            print(f"App connected from: {address}")
            
            while True:
                try:
                    cmd_buffer = client_socket.recv(1024).decode(encoding="utf-8")
                    if not cmd_buffer:
                        break
                    
                    if self.g_debug: print(f"TCP RX Buffer: {cmd_buffer.strip()}")
                    
                    # 处理粘包问题，找出所有完整的指令
                    while '$' in cmd_buffer and '#' in cmd_buffer:
                        start_index = cmd_buffer.find('$')
                        end_index = cmd_buffer.find('#', start_index)
                        if end_index != -1:
                            command = cmd_buffer[start_index : end_index + 1]
                            self.parse_data(client_socket, command)
                            cmd_buffer = cmd_buffer[end_index + 1:]
                        else:
                            break # 没有找到结束符，等待更多数据
                except Exception as e:
                    print(f"TCP connection error: {e}")
                    break
            
            client_socket.close()
            self.g_socket = None
            print("App disconnected.")
            self.g_mode = 'Home' # 客户端断开后重置模式

    def init_tcp_socket(self):
        if self.g_init:
            return
            
        while True:
            ip = self.g_wifi.get_ip_address()
            if ip != "x.x.x.x":
                self.g_tcp_ip = ip
                print(f"TCP Service IP found: {ip}")
                break
            print("Acquiring IP address...")
            time.sleep(1)

        task_tcp = threading.Thread(target=self.start_tcp_server, name="task_tcp", args=(ip, 6000))
        task_tcp.setDaemon(True)
        task_tcp.start()
        if self.g_debug:
            print('--- TCP Socket Thread Initialized ---')

    # --------------------------------------------------------------------------
    # 雷达相关功能
    # --------------------------------------------------------------------------
    def laser_avoidance(self):
        docker_script = "cd ~ && ./run_docker.sh"
        
        # 1. 启动容器（新终端）
        subprocess.Popen(["gnome-terminal", "--", "bash", "-c", docker_script])
        time.sleep(3)  # 等待容器启动
        
        # 2. 获取最新容器 ID
        result = subprocess.run(
            ["docker", "ps", "-lq"],
            stdout=subprocess.PIPE,
            text=True
        )
        container_id = result.stdout.strip()
        
        if not container_id:
            print("未找到运行的容器！")
            return
        
        self.g_laser_container_id = container_id  # 保存容器ID
        print(f"容器 ID: {container_id}")
        
        # 3. 在每个新终端中执行 ROS2 命令并保持终端运行
        commands = [
            "ros2 run icar_bringup Mcnamu_driver_X3",
            "ros2 launch sllidar_ros2 sllidar_launch.py",
            "ros2 run icar_laser laser_Avoidance_a1_X3"
        ]
        
        for cmd in commands:
            # 使用更明确的方式确保命令在容器内执行
            process = subprocess.Popen([
                "gnome-terminal",
                "--title", f"ROS2: {cmd}",
                "--",
                "docker", "exec", "-it", container_id, "bash", "-lic", f"{cmd}; exec bash"
            ])
            
            self.g_laser_processes.append(process)  # 保存进程引用
            time.sleep(1)
            print(f"[DEBUG] 命令 {cmd} 已在容器内新终端运行")
    
    def avoidance_close(self):
        # 1. 关闭保存的 gnome-terminal 进程（尤其是 laser_Avoidance_a1_X3）
        for proc in getattr(self, "g_laser_processes", []):
            try:
                print(f"[INFO] 终止进程 PID: {proc.pid}")
                os.kill(proc.pid, signal.SIGTERM)  # 优雅终止
            except Exception as e:
                print(f"[WARNING] 无法终止进程 {proc.pid}: {e}")

        self.g_laser_processes.clear()

        # 2. 停止并移除 Docker 容器
        container_id = getattr(self, "g_laser_container_id", "")
        if container_id:
            try:
                print(f"[INFO] 停止并移除容器: {container_id}")
                subprocess.run(["docker", "stop", container_id])
                subprocess.run(["docker", "rm", container_id])
            except Exception as e:
                print(f"[ERROR] 停止或删除容器失败: {e}")
            
            self.g_laser_container_id = ""
            self.g_laser_avoidance = False  # 重置状态
        else:
            print("[INFO] 无容器 ID 可关闭")

        print("[SUCCESS] 所有资源已尝试释放")
        
    def laser_warning(self):
        docker_script = "cd ~ && ./run_docker.sh"
        
        # 1. 启动容器（新终端）
        subprocess.Popen(["gnome-terminal", "--", "bash", "-c", docker_script])
        time.sleep(3)  # 等待容器启动
        
        # 2. 获取最新容器 ID
        result = subprocess.run(
            ["docker", "ps", "-lq"],
            stdout=subprocess.PIPE,
            text=True
        )
        container_id = result.stdout.strip()
        
        if not container_id:
            print("未找到运行的容器！")
            return
        
        self.g_laser_container_id = container_id  # 保存容器ID
        print(f"容器 ID: {container_id}")
        
        # 3. 在每个新终端中执行 ROS2 命令并保持终端运行
        commands = [
            "ros2 run icar_bringup Mcnamu_driver_X3",
            "ros2 launch sllidar_ros2 sllidar_launch.py",
            "ros2 run icar_laser laser_Warning_a1_X3"
        ]
        
        for cmd in commands:
            # 使用更明确的方式确保命令在容器内执行
            process = subprocess.Popen([
                "gnome-terminal",
                "--title", f"ROS2: {cmd}",
                "--",
                "docker", "exec", "-it", container_id, "bash", "-lic", f"{cmd}; exec bash"
            ])
            
            self.g_laser_processes.append(process)  # 保存进程引用
            time.sleep(1)
            print(f"[DEBUG] 命令 {cmd} 已在容器内新终端运行")
    
    def warning_close(self):
        # 1. 关闭保存的 gnome-terminal 进程（尤其是 laser_Warning_a1_X3）
        for proc in getattr(self, "g_laser_processes", []):
            try:
                print(f"[INFO] 终止进程 PID: {proc.pid}")
                os.kill(proc.pid, signal.SIGTERM)  # 优雅终止
            except Exception as e:
                print(f"[WARNING] 无法终止进程 {proc.pid}: {e}")

        self.g_laser_processes.clear()

        # 2. 停止并移除 Docker 容器
        container_id = getattr(self, "g_laser_container_id", "")
        if container_id:
            try:
                print(f"[INFO] 停止并移除容器: {container_id}")
                subprocess.run(["docker", "stop", container_id])
                subprocess.run(["docker", "rm", container_id])
            except Exception as e:
                print(f"[ERROR] 停止或删除容器失败: {e}")
            
            self.g_laser_container_id = ""
            self.g_laser_warning = False  # 重置状态
        else:
            print("[INFO] 无容器 ID 可关闭")

        print("[SUCCESS] 所有资源已尝试释放")

    def laser_tracker(self):
        docker_script = "cd ~ && ./run_docker.sh"
        
        # 1. 启动容器（新终端）
        subprocess.Popen(["gnome-terminal", "--", "bash", "-c", docker_script])
        time.sleep(3)  # 等待容器启动
        
        # 2. 获取最新容器 ID
        result = subprocess.run(
            ["docker", "ps", "-lq"],
            stdout=subprocess.PIPE,
            text=True
        )
        container_id = result.stdout.strip()
        
        if not container_id:
            print("未找到运行的容器！")
            return
        
        self.g_laser_container_id = container_id  # 保存容器ID
        print(f"容器 ID: {container_id}")
        
        # 3. 在每个新终端中执行 ROS2 命令并保持终端运行
        commands = [
            "ros2 run icar_bringup Mcnamu_driver_X3",
            "ros2 launch sllidar_ros2 sllidar_launch.py",
            "ros2 run icar_laser laser_Tracker_a1_X3"
        ]
        
        for cmd in commands:
            # 使用更明确的方式确保命令在容器内执行
            process = subprocess.Popen([
                "gnome-terminal",
                "--title", f"ROS2: {cmd}",
                "--",
                "docker", "exec", "-it", container_id, "bash", "-lic", f"{cmd}; exec bash"
            ])
            
            self.g_laser_processes.append(process)  # 保存进程引用
            time.sleep(1)
            print(f"[DEBUG] 命令 {cmd} 已在容器内新终端运行")
    
    def tracker_close(self):
        # 1. 关闭保存的 gnome-terminal 进程（尤其是 laser_Tracker_a1_X3）
        for proc in getattr(self, "g_laser_processes", []):
            try:
                print(f"[INFO] 终止进程 PID: {proc.pid}")
                os.kill(proc.pid, signal.SIGTERM)  # 优雅终止
            except Exception as e:
                print(f"[WARNING] 无法终止进程 {proc.pid}: {e}")

        self.g_laser_processes.clear()

        # 2. 停止并移除 Docker 容器
        container_id = getattr(self, "g_laser_container_id", "")
        if container_id:
            try:
                print(f"[INFO] 停止并移除容器: {container_id}")
                subprocess.run(["docker", "stop", container_id])
                subprocess.run(["docker", "rm", container_id])
            except Exception as e:
                print(f"[ERROR] 停止或删除容器失败: {e}")
            
            self.g_laser_container_id = ""
            self.g_laser_tracker = False  # 重置状态
        else:
            print("[INFO] 无容器 ID 可关闭")

        print("[SUCCESS] 所有资源已尝试释放")
