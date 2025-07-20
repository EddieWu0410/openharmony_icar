import cv2
import numpy as np
import socket
from self_detect import YoloDetecter, parse_opt
import time
import sys
import threading 
import struct


def receive_image(conn):
    # 接收图片大小
    data_len_str = conn.recv(16).decode().strip()
    data_len = int(data_len_str)

    # 接收图片数据
    data = b''
    while len(data) < data_len:
        packet = conn.recv(min(data_len - len(data), 1024))
        if not packet:
            break
        data += packet

    # 解码图片
    decoded_data = np.frombuffer(data, dtype=np.uint8)
    image = cv2.imdecode(decoded_data, cv2.IMREAD_COLOR)

    # 显示图片
    cv2.imshow('Received Image', image)
    cv2.waitKey(2)
    cv2.destroyAllWindows()

    return image


def handle_client(client_socket):
    length = int.from_bytes(client_socket.recv(4), byteorder='big')
    stringData = client_socket.recv(length)

    data = np.frombuffer(stringData, dtype='uint8')
    decimg = cv2.imdecode(data,1)
    return decimg


def receive_frame(conn):
    """接收客户端发送的视频帧"""
    try:
        # 接收帧数据长度（4字节）
        data_len_bytes = conn.recv(4)
        if len(data_len_bytes) < 4:
            return None
        
        data_len = struct.unpack('!I', data_len_bytes)[0]
        
        # 接收完整的帧数据
        data = b''
        while len(data) < data_len:
            packet = conn.recv(min(data_len - len(data), 4096))
            if not packet:
                return None
            data += packet
        
        # 解码图像
        nparr = np.frombuffer(data, np.uint8)
        frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
        if frame is None:
            print("解码失败，收到的数据长度：", len(data))
        return frame
        
    except Exception as e:
        print(f"接收帧数据出错: {e}")
        return None


def send_frame(conn, frame):
    """发送处理后的视频帧给客户端"""
    try:
        # 编码图像
        _, encoded_frame = cv2.imencode('.jpg', frame, [cv2.IMWRITE_JPEG_QUALITY, 80])
        frame_data = encoded_frame.tobytes()
        
        # 发送数据长度
        data_len = len(frame_data)
        conn.send(struct.pack('!I', data_len))
        
        # 发送帧数据
        conn.send(frame_data)
        return True
        
    except Exception as e:
        print(f"发送帧数据出错: {e}")
        return False


def process_client_video_stream(conn, addr, self_yolo, font):
    """处理单个客户端的视频流 - 使用原始代码的处理逻辑"""
    print(f"客户端 {addr} 已连接，开始处理视频流")
    
    window_name = f'Client_{addr[0]}_{addr[1]}_Result'
    raw_window_name = f'Client_{addr[0]}_{addr[1]}_Raw'
    
    try:
        while True:
            # 接收客户端视频帧
            frame = receive_frame(conn)
            print("服务端收到帧均值：", np.mean(frame) if frame is not None else "None")
            print("接收到")
            if frame is None:
                print(f"客户端 {addr} 断开连接或数据接收失败")
                break

            # YOLO目标检测
            res_img, yolo_list = self_yolo.detect(frame)
            pixel_list = self_yolo.yolo_to_pixel(yolo_list, res_img.shape[0], res_img.shape[1])

            # 处理检测结果
            i = 1
            msg = ''
            for e in pixel_list:
                cv2.circle(frame, (int(e[0]), int(e[1])), 2, (255,0,0), -1)
                cv2.putText(frame, str(e[2]), (int(e[0])+5, int(e[1])), font, 0.5, (0,0,255), 2)
                cv2.putText(frame, str(e[3]), (int(e[0])+5, int(e[1])+10), font, 0.5, (0,0,255), 2)
                if i > 1:
                    msg = msg + ';'
                msg = msg + str(e[0]) + ',' + str(e[1]) + ',' + str(e[3]) + ',' + str(time.time())
                i = i + 1

            print(f"客户端 {addr} 检测结果: {msg}")

            # 显示处理结果（服务端监控）
            try:
                cv2.imshow(window_name, frame)
                cv2.waitKey(1)
            except:
                print(f"无法显示窗口: {window_name}")

            # 同时发送处理后的视频帧回客户端
            if not send_frame(conn, frame):
                print(f"向客户端 {addr} 发送视频帧失败")
                break

    except Exception as e:
        print(f"处理客户端 {addr} 视频流时出错: {e}")
    finally:
        try:
            cv2.destroyWindow(raw_window_name)
        except:
            pass
        try:
            cv2.destroyWindow(window_name)
        except:
            pass
        try:
            conn.close()
        except:
            pass
        print(f"客户端 {addr} 连接已关闭")


if __name__ == "__main__":
    # 服务器配置
    host = '0.0.0.0'
    port = 12345
    
    print(f"启动视频流处理服务器: {host}:{port}")
    server_address = (host, port)

    # 初始化YOLO检测器
    opt = parse_opt()
    self_yolo = YoloDetecter(**vars(opt))
    font = cv2.FONT_HERSHEY_SIMPLEX
    
    # 创建socket服务器
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(server_address)
        s.listen(5)  # 支持多个客户端同时连接

        print(f"服务器正在监听连接: {host}:{port}")
        print("等待客户端连接...")
        
        client_threads = []
        
        try:
            while True:
                conn, addr = s.accept()
                print(f"新客户端连接: {addr}")
                
                # 为每个客户端创建独立的处理线程
                client_thread = threading.Thread(
                    target=process_client_video_stream, 
                    args=(conn, addr, self_yolo, font)
                )
                client_thread.daemon = True
                client_thread.start()
                client_threads.append(client_thread)
                
                # 清理已结束的线程
                client_threads = [t for t in client_threads if t.is_alive()]
                
        except KeyboardInterrupt:
            print("\n服务器正在关闭...")
        except Exception as e:
            print(f"服务器错误: {e}")
        finally:
            # 等待所有客户端线程结束
            for thread in client_threads:
                if thread.is_alive():
                    thread.join(timeout=1.0)
            
            cv2.destroyAllWindows()
            print("服务器已关闭")
