from flask import Flask, request, jsonify
from flask_cors import CORS
import socket

app = Flask(__name__)
CORS(app)

BACKEND_IP = "192.168.5.165"
BACKEND_PORT = 6000


def send_tcp_command(command):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((BACKEND_IP, BACKEND_PORT))
            s.sendall(command.encode('utf-8'))
            return True
    except Exception as e:
        return False, str(e)


@app.route('/send-command', methods=['POST'])
def send_command():
    data = request.get_json()
    command = data.get('command')
    print("收到的：", command)

    if not command:
        return jsonify({"success": False, "error": "Missing command"}), 400

    success = send_tcp_command(command)
    if success:
        return jsonify({"success": True})
    else:
        return jsonify({"success": False}), 500


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
