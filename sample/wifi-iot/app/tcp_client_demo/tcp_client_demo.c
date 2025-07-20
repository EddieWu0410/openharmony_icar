#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#include <hi_timer.h>
#include <time.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
// wifi 头文件
#include "wifi_device.h"
#include "lwip/netifapi.h"
#include "lwip/api_shell.h"
#include "lwip/sockets.h"
#include "./wifi/wifi_connect.h"

#define TASK_STACK_SIZE (1024 * 10)
#define TASK_DELAY_10S 10
#define CONFIG_WIFI_SSID "ohcode"          // 要连接的WiFi 热点账号
#define CONFIG_WIFI_PWD "88888888"        // 要连接的WiFi 热点密码
#define CONFIG_SERVER_IP "192.168.149.20" // 要连接的服务器IP
#define CONFIG_SERVER_PORT 8888           // 要连接的服务器端口

static void TCPClientTask(void)
{
    int socket_fd = 0;
    int ret = 0;
    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);

    socket_fd = socket(AF_INET, SOCK_STREAM, 0); // 创建套接字（TCP）
    if (socket_fd < 0)
    {
        perror("[actuator_service]sock_fd create error\r\n");
        return;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CONFIG_SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr(CONFIG_SERVER_IP); // 填写服务器的IP地址

    ret = connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr)); // 连接服务器
    if (ret == -1)
    {
        printf("Failed to connect to the server\r\n");
        return;
    }
    printf("Connection to server successful\r\n");

    char msg[1024];
    int cnt = 0;
    while (1)
    {
        sprintf(msg, "hello %d\r\n", cnt++);
        ret = send(socket_fd, msg, strlen(msg), 0);

        if (ret == -1)
        {
            printf("send error\r\n");
            break;
        }
        osDelay(100); //10ms*100=1s
    }
}

static void TCPClientDemo(void)
{
    osThreadAttr_t attr;

    attr.name = "TCPClientTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = TASK_STACK_SIZE;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)TCPClientTask, NULL, &attr) == NULL)
    {
        printf("[UDPClientDemo] Failed to create UDPClientTask!\n");
    }
}

APP_FEATURE_INIT(TCPClientDemo);