#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "MQTTClient.h"     //mqtt三方库
#include "cJSON.h"          //cJson解析三方库
#include "wifi_connecter.h" // easy wifi (station模式)
#include "iot_gpio.h"
#include "iot_uart.h"
#include "hi_io.h" // 海思 Pegasus SDK：IoT硬件设备操作接口-IO

/*******************************************define***********************************************/
static unsigned char sendBuf[2000];
static unsigned char readBuf[2000];
#define IOT_UART_IDX_2 (2)
#define STACK_SIZE (1024)
#define DELAY_US (100000)
#define IOT_GPIO_11 (11)
#define IOT_GPIO_12 (12)

// 定义用于mqtt连接的两个对象变量，对象类型来自mqtt三方库
Network network;
MQTTClient client;
unsigned char uartReadBuff[2048] = {0};
unsigned char uartWriteBuff[] = "hello uart!";

/*******************************************WIFI参数（请根据实际情况修改）***********************************************/
// 定义一个宏，用于标识SSID。
#define PARAM_HOTSPOT_SSID "openharmony"
// 定义一个宏，用于标识密码。
#define PARAM_HOTSPOT_PSK "88888888"
// 定义一个宏，用于标识加密方式
#define PARAM_HOTSPOT_TYPE WIFI_SEC_TYPE_PSK

/*******************************************MQTT参数（请根据实际情况修改）***********************************************/
// 定义一个宏，用于标识MQTT服务器IP地址。
#define HOST_ADDR "9f233003dc.st1.iotda-device.cn-north-4.myhuaweicloud.com" // hostname
// 定义一个宏，用于标识MQTT服务器端口
#define HOST_PORT 1883
// 定义一个宏，用于标识用于MQTT连接的设备ID
#define DEVICE_ID "686f93ba32771f177b4ab0e9_Car_test_001"
// 定义一个宏，用于标识用于MQTT连接的clientid
#define MQTT_CLIENT_ID "686f93ba32771f177b4ab0e9_Car_test_001_0_0_2025071311"
// 定义一个宏，用于标识用于MQTT连接的username
#define MQTT_USERNAME "686f93ba32771f177b4ab0e9_Car_test_001"
// 定义一个宏，用于标识用于MQTT连接的password
#define MQTT_PASSWORD "29800f8509dea0831fa3bb26b2f34e878a6b43e052367cfe8fa407d52c2172e2"
// 定义一个宏，用于标识设备属性上报所用的topic
#define PUBLISH_TOPIC "$oc/devices/" DEVICE_ID "/sys/properties/report"
// 定义一个宏，用于标识订阅下发命令所用的topic
#define SUBCRIB_TOPIC "$oc/devices/" DEVICE_ID "/sys/commands/#"
// 定义一个宏，用于标识执行完成下发命令所返回响应的topic
#define RESPONSE_TOPIC "$oc/devices/" DEVICE_ID "/sys/commands/response"

/*******************************************function***********************************************/
// 云端下发的控制命令响应函数
void messageArrived(MessageData *data)
{
    int rc;
    printf("Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len, data->topicName->lenstring.data,
           data->message->payloadlen, data->message->payload);
    // 此处处理收到云端发送过来的命令，在此处编写处理命令的业务逻辑，比如开灯、开门等
    // 智能水杯只有数据上传，没有控制命令，无需编写此处业务逻辑
}

int usr_uart_config(void)
{
    // 初始化UART配置，波特率115200，数据bit为8, 停止位1，奇偶校验为NONE，流控为NONE
    IotUartAttribute g_uart_cfg = {115200, 8, 1, IOT_UART_PARITY_NONE, 500, 500, 0};
    int ret = IoTUartInit(IOT_UART_IDX_2, &g_uart_cfg);
    if (ret != 0)
    {
        printf("uart init fail\r\n");
    }
    return ret;
}

static void MQTTDemoTask(void)
{

    // 串口初始化
    printf("[UartDemo] UartDemo_Task()\n");
    IoTGpioInit(IOT_GPIO_11); // 使用GPIO，都需要调用该接口
    IoTGpioInit(IOT_GPIO_12); // 使用GPIO，都需要调用该接口
    hi_io_set_func(IOT_GPIO_11, HI_IO_FUNC_GPIO_11_UART2_TXD);
    hi_io_set_func(IOT_GPIO_12, HI_IO_FUNC_GPIO_12_UART2_RXD);
    printf("UART init...\r\n");
    usr_uart_config();

    printf(">> MQTTDemoTask ...\n");
    // 定义热点配置
    WifiDeviceConfig config = {0};
    // 设置热点配置中的SSID
    strcpy(config.ssid, PARAM_HOTSPOT_SSID);
    // 设置热点配置中的密码
    strcpy(config.preSharedKey, PARAM_HOTSPOT_PSK);
    // 设置热点配置中的加密方式
    config.securityType = PARAM_HOTSPOT_TYPE;
    // 等待100ms
    osDelay(10);
    // 连接到热点
    int netId = ConnectToHotspot(&config);
    // 检查是否成功连接到热点
    if (netId < 0)
    {
        printf("Connect to AP failed!\r\n");
        return;
    }

    // 初始化并启动MQTT任务，连接MQTT服务器
    int rc, count = 0;
begin:
    NetworkInit(&network);
    NetworkConnect(&network, HOST_ADDR, HOST_PORT);
    printf("MQTTClientInit  ...\n");
    MQTTClientInit(&client, &network, 2000, sendBuf, sizeof(sendBuf), readBuf, sizeof(readBuf));
    MQTTString clientId = MQTTString_initializer;
    clientId.cstring = MQTT_CLIENT_ID;
    MQTTString userName = MQTTString_initializer;
    userName.cstring = MQTT_USERNAME;
    MQTTString password = MQTTString_initializer;
    password.cstring = MQTT_PASSWORD;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.clientID = clientId;
    data.username = userName;
    data.password = password;
    data.willFlag = 0;
    data.MQTTVersion = 4;
    data.keepAliveInterval = 60;
    data.cleansession = 1;
    printf("MQTTConnect  ...\n");
    rc = MQTTConnect(&client, &data);
    if (rc != 0)
    {
        printf("MQTTConnect: %d\n", rc);
        NetworkDisconnect(&network);
        MQTTDisconnect(&client);
        osDelay(200);
        goto begin;
    }

    printf("MQTTSubscribe  ...\n");
    rc = MQTTSubscribe(&client, SUBCRIB_TOPIC, 0, messageArrived);

    if (rc != 0)
    {
        printf("MQTTSubscribe: %d\n", rc);
        NetworkDisconnect(&network);
        MQTTDisconnect(&client);
        osDelay(200);
        goto begin;
    }
    else
    {
        printf("MQTTSubscribe succ  ...\n");
    }
    MQTTMessage message;
    while (1)
    {
        message.qos = 0;
        message.retained = 0;
        message.payload = uartReadBuff;
        message.payloadlen = strlen(uartReadBuff);

        if ((rc = MQTTPublish(&client, PUBLISH_TOPIC, &message)) != 0)
        {
            printf("Return code from MQTT publish is %d\n", rc);
            NetworkDisconnect(&network);
            MQTTDisconnect(&client);
            osDelay(200);
            goto begin;
        }
        else
        {
            printf("mqtt publish success:%s\n", uartReadBuff);
            memset(uartReadBuff, 0, sizeof(uartReadBuff));
        }

        if ((rc = MQTTYield(&client, 5000)) != 0)
        {
            printf("[ERR] yield rc=%d, reconnecting...\n", rc);
            // 正常流程先断开
            MQTTDisconnect(&client);
            NetworkDisconnect(&network);
            // 跳到begin
            goto begin;
        }
    }
}

void recv_task()
{
    while (1)
    {
        int len = IoTUartRead(IOT_UART_IDX_2, uartReadBuff, sizeof(uartReadBuff));
        if (len > 0)
        {
            printf("Uart read data:%s  len:%d\r\n", uartReadBuff, len);
        }
        osDelay(100);
    }
}

static void OC_Demo(void)
{
    osThreadAttr_t attr;

    attr.name = "MQTTDemoTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = 24;

    if (osThreadNew((osThreadFunc_t)MQTTDemoTask, NULL, &attr) == NULL)
    {
        printf("Falied to create MQTTDemoTask!\n");
    }

    attr.name = "recv_task";
    attr.stack_size = 1024 * 5;
    if (osThreadNew((osThreadFunc_t)recv_task, NULL, &attr) == NULL)
    {
        printf("Falied to create recv_task!\n");
    }
}

APP_FEATURE_INIT(OC_Demo);