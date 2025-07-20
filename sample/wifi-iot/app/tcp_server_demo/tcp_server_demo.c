#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "cJSON.h"
#include "cmsis_os2.h"
#include "ohos_init.h"
#include "wifi_event.h"
#include "lwip/sockets.h"
#include "./wifi/wifi_connect.h"

#define TASK_STACK_SIZE (1024 * 10)
#define TASK_DELAY_2S 2
#define CONFIG_WIFI_SSID "ohcode"
#define CONFIG_WIFI_PWD "88888888"
#define CONFIG_CLIENT_PORT 8888
#define TCP_BACKLOG 10

static void TCPServerTask(void)
{
    // 在sock_fd 进行监听，在 new_fd 接收新的链接
    int sock_fd, new_fd;
    // 服务端地址信息
    struct sockaddr_in server_sock;
    // 客户端地址信息
    struct sockaddr_in client_sock;
    // 接收数据长度
    int sin_size;
    // 客户端地址信息指针
    struct sockaddr_in *cli_addr;
    // 连接Wifi
    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);

   //创建socket
	if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket is error\r\n");
		exit(1);
	}

    bzero(&server_sock, sizeof(server_sock));
	server_sock.sin_family = AF_INET;
	server_sock.sin_addr.s_addr = htonl(INADDR_ANY);
	server_sock.sin_port = htons(CONFIG_CLIENT_PORT);
    // 绑定接收 socket
    if (bind(sock_fd, (struct sockaddr *)&server_sock, sizeof(struct sockaddr)) == -1)
    {
        perror("bind error");
        close(sock_fd);
        return;
    }

    // 监听接收 socket
    if (listen(sock_fd, TCP_BACKLOG) == -1)
    {
        perror("listen error");
        close(sock_fd);
        return;
    }

    printf("start accept on port %d\n", CONFIG_CLIENT_PORT);

    sin_size = sizeof(struct sockaddr_in);

    while (1)
    {
        // 接收客户端连接
        if ((new_fd = accept(sock_fd, (struct sockaddr *)&client_sock, (socklen_t *)&sin_size)) == -1)
        {
            perror("accept client error\r\n");
            continue;
        }
        printf("client connect: %s\r\n", inet_ntoa(((struct sockaddr_in *)(&client_sock))->sin_addr));
        char recv_buf[1024];
        int recv_len;
        while (1)
        {
            recv_len = recv(new_fd, recv_buf, sizeof(recv_buf), 0);
            if (recv_len <= 0)
            {
                break;
            }
            char recv_data[recv_len];
            memcpy(recv_data, recv_buf, recv_len);
            recv_data[recv_len] = '\0';
            printf("len: %d data: %s\r\n", recv_len, recv_data);
        }
    }
}

static void TCPServerDemo(void)
{
    osThreadAttr_t attr;
    attr.name = "TCPServerTask";
    attr.stack_size = TASK_STACK_SIZE;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)TCPServerTask, NULL, &attr) == NULL)
    {
        printf("Failed to create TCP server task\n");
    }
}

SYS_RUN(TCPServerDemo);