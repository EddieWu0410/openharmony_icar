from client_conf import ClientConf
from mqtt_client import MqttClient
import json
from datetime import datetime
from flask import Flask, jsonify
from flask_cors import CORS
import threading

app = Flask(__name__)
CORS(app)

latest_sensor_data = None

def handle_message(message):
    try:
        global latest_sensor_data
        data = json.loads(message)
        if 'notify_data' in data and 'body' in data['notify_data']:
            services = data['notify_data']['body']['services']
            if services and len(services) > 0:
                service_data = services[0]['properties']
                latest_sensor_data = {
                    'temperature': float(service_data.get('temperature', 0)),
                    'humidity': float(service_data.get('humidity', 0)), 
                    'illumination': float(service_data.get('illumination', 0)),
                    'smoke': float(service_data.get('smoke', 0)),
                    'pressure': float(service_data.get('pressure', 0)),
                    'longitude': float(service_data.get('longitude', 116.3974)),
                    'latitude': float(service_data.get('latitude', 39.9093)),
                    'battery': float(service_data.get('battery', 0)),
                    'pm25': float(service_data.get('pm25', 0)),
                    'timestamp': datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                }
                print("收到新数据:", latest_sensor_data)
                return latest_sensor_data
    except Exception as e:
        print(f"处理消息出错: {str(e)}")
    return None

@app.route('/sensor-data')
def get_sensor_data():
    if latest_sensor_data:
        return jsonify(latest_sensor_data)
    return jsonify({'error': 'No data available'})

def run_flask():
    app.run(host='0.0.0.0', port=5050)

def main():
    # 配置MQTT客户端
    client_conf = ClientConf()
    client_conf.host = "9f233003dc.st1.iotda-app.cn-north-4.myhuaweicloud.com" 
    client_conf.port = 8883
    client_conf.topic = "sensorData"
    client_conf.access_key = "BG7xUAlG"
    client_conf.access_code = "OjigB599iFbYzIHOf3ZXJTwwR9gmWHL0"
    client_conf.instance_id = "74aa2e95-6ba7-4528-a41a-04a4f03cb1c8"
    
    mqtt_client = MqttClient(client_conf)
    
    if mqtt_client.connect() != 0:
        print("MQTT连接失败")
        return
        
    mqtt_client.set_message_callback(handle_message)
    
    # 启动Flask服务
    threading.Thread(target=run_flask, daemon=True).start()
    
    try:
        while True:
            import time
            time.sleep(1)
    except KeyboardInterrupt:
        print("正在关闭服务...")
        mqtt_client.close()

if __name__ == "__main__":
    main()