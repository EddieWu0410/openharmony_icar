#!/usr/bin/env python3
# coding=utf-8
# Final App Server with WebSocket Proxy (Gemini)

import os
import sys
import threading
import time
import socket
from flask import Flask, render_template, Response, request
from flask_sock import Sock
import rosmaster_main 

app = Flask(__name__, template_folder='./templates')
sock = Sock(app)

is_debug = "debug" in sys.argv
myApp = rosmaster_main.MyRosmasterApp(debug=is_debug)


@app.route('/')
def index():
    """主页，提供连接信息和控制页面入口。"""
    return render_template('index.html')


@app.route('/index2')
def index2():
    """控制页面，包含视频流和控制按钮。"""
    return render_template('index2.html')


@app.route('/video_feed')
def video_feed():
    """
    视频流路由。
    直接调用机器人逻辑类中的视频流处理生成器。
    """
    return Response(myApp.mode_handle(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')


@app.route('/init')
def init():
    """初始化路由，确保后端的TCP指令服务已启动。"""
    print("--- HTTP /init endpoint called. Initializing TCP command server... ---")
    if not myApp.g_init:
        myApp.init_tcp_socket()
        return "TCP Server Initialized."
    return "TCP Server is already running."


@app.route('/yolo_video_feed')
def yolo_video_feed():
    """
    YOLO处理后的视频流路由
    从单独的processed_frame变量中获取YOLO处理后的帧
    """
    return Response(myApp.yolo_mode_handle(),
                    mimetype='multipart/x-mixed-replace; boundary=frame')
    

@app.route('/yolo_detailed_status')
def yolo_detailed_status():
    """获取YOLO视频流的详细状态"""
    try:
        status = myApp.get_yolo_stream_status()
        return jsonify({
            'status': 'success',
            'data': status,
            'message': 'YOLO stream is running' if status['enabled'] else 'YOLO stream is stopped'
        })
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': f'Error getting detailed YOLO status: {str(e)}'
        }), 500


# --- WebSocket 代理路由 ---

@sock.route('/ws/control')
def control_socket(ws):
    """
    WebSocket to TCP 代理。
    接收来自浏览器的WebSocket消息，并转发到后端的TCP指令服务器。
    """
    print(f"WebSocket client connected from: {request.remote_addr}")
    tcp_client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        # 连接到内部的TCP指令服务器
        tcp_client.connect(('127.0.0.1', 6000))
        print("WebSocket proxy successfully connected to internal TCP server on port 6000.")
        
        while True:
            # 等待从浏览器接收指令
            command = ws.receive()
            if command is None:
                # 浏览器端关闭了连接
                break
            
            if myApp.g_debug:
                print(f"WS RX (from browser): {command}")
            
            # 将指令通过TCP发送给机器人后台
            tcp_client.sendall(command.encode('utf-8'))

    except Exception as e:
        print(f"An error occurred in the WebSocket proxy: {e}")
    finally:
        print("WebSocket client disconnected. Closing TCP connection to command server.")
        tcp_client.close()


# --- 主程序入口 ---
if __name__ == '__main__':
    print("--- Starting Rosmaster Web Application Server ---")
    
    # 启动机器人后台的TCP指令服务线程
    myApp.init_tcp_socket()

    myApp.start_video_server_stream(
        host=myApp.video_server_host, 
        port=myApp.video_server_port
    )

    # 播放启动提示音，给用户反馈
    time.sleep(.1)
    for i in range(3):
        myApp.g_bot.set_beep(60)
        time.sleep(.2)

    # 打印版本和访问信息
    version = myApp.g_bot.get_version()
    print(f"ROSmaster STM32 Version: {version}")
    print("Web App Server is running. Waiting for connection from Browser.")
    print("Please open your browser and navigate to http://<your_car_ip>:6500")

    try:
        # 启动Flask Web服务器
        app.run(host='0.0.0.0', port=6500, threaded=True, debug=False)
    except KeyboardInterrupt:
        print("\n--- Server shutting down. Cleaning up resources... ---")
        myApp.g_bot.set_car_motion(0, 0, 0)
        myApp.g_bot.set_beep(0)
        # 在退出前释放资源
        del myApp
        print("Cleanup complete. Goodbye.")
