# openharmony_icar
---
北京交通大学软件学院实训项目 - 基于OpenHarmony的智能小车

## 项目结构
```
.
├── hihope
│   └── HIHOPE-HH-SLHSP104
│       ├── demo
│       │   └── all_my_car_down
│       │       ├── app
│       │       ├── bsp
│       │       ├── bsp_beep
│       │       ├── sensor
│       │       ├── uart7_gps
│       │       ├── uartcall
│       │       ├── usart1
│       │       └── usart3
│       ├── hals
│       └── kernel_configs
│
├── icar_web
│   ├── demo_integration
│   ├── icar_back
│   └── icar_front
│
└── sample
    ├── camera
    └── wifi-iot
        ├── app
        │   ├── at32_wifi
        │   ├── demolink
        │   ├── iothardware
        │   ├── samgr
        │   ├── startup
        │   ├── tcp_client_demo
        │   │   └── wifi
        │   ├── tcp_server_demo
        │   │   └── wifi
        │   └── wifi_demo
        └── library
```

## 项目说明

### hihope
- 本目录包含下位机AT32主板的代码，涵盖马达控制、蜂鸣器操作、各类传感器数据采集及上下位机通信等关键驱动实现
- 核心代码位于 `hihope/HIHOPE-HH-SLHSP104/demo/all_my_car_down` 目录下
- 使用说明
    - 将 `openharmony/vendor` 目录下的 `hihope` 目录替换为本项目中的目录
    - 执行全量编译命令
    ``` bash
    rm -rf out && ./build.sh --product-name HIHOPE-HH-SLHSP104 --ccache --no-prebuilt-sdk
    ```
    - 将编译生成的固件烧录至AT32主板

### icar_web
- 本目录包含Web端代码，包括基于React的前端、Flask后端及上位机的相关代码
- `demo_integration` 目录对上位机中部分代码进行了修改，以支持Web端功能集成：
    - 将 `app.py` 和 `rosmaster_main.py` 上传至上位机的 `~/Rosmaster-App/rosmaster` 目录
    - 注意需将 `~/Rosmaster-App/rosmaster` 目录下 `rosmaster_main.cpython-38-aarch64-linux-gnu.so` 文件重命名为 `rosmaster_main.cpython-38-aarch64-linux-gnu.so.back`，以确保 `rosmaster_main.py` 生效
    - `aiserver.py` 上传至上位机的 `~/yolov7` 目录
    - 目前支持红外巡线、雷达避障、雷达警戒、雷达追踪、yolo目标检测和鸣笛六个功能的一键开启和关闭。
- `icar_back` Web端后端代码，实现华为云IoTDA数据接收及前端控制命令转发功能
    - 运行以下命令以安装所需依赖
    ```bash
    pip install -r requirements.txt
    ```
    - 注意修改ip和mqtt客户端配置信息
- `icar_front` Web端前端代码，实现控制指令下发、实时视频流渲染及传感器数据看板功能
    - 运行以下命令安装库
    ```bash
    npm install
    ```
    - 启动项目
    ```bash
    npm run dev
    ```

### sample
- 本目录包含下位机WiFi模块的代码，负责传感器数据接收及数据上云等核心实现
- 核心代码位于 `sample/wifi-iot/app` 目录下
- 使用说明
    - 将 `openharmony/applications` 目录下的 `sample` 目录替换为本项目中的目录
    - 执行全量编译命令
    ```bash
    rm -rf out && ./build.sh --product-name wifiiot_hispark_pegasus --ccache --no-prebuilt-sdk
    ```
    - 将编译生成的固件烧录至WiFi模块
