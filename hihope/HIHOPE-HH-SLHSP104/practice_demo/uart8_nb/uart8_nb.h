#ifndef _UART8_NB_H_
#define _UART8_NB_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_usart.h"

/***********************UART8配置参数****************************/
#define GPIO_CLK CRM_GPIOE_PERIPH_CLOCK
#define UART_CLK CRM_UART8_PERIPH_CLOCK
#define UART_NUM UART8
#define GPIO_TX GPIOE
#define GPIO_RX GPIOE
#define PIN_TX GPIO_PINS_1
#define PIN_RX GPIO_PINS_0

#define UART_NUM_IRQn UART8_IRQn
#define UART_NUM_IRQ_FUN() UART8_IRQHandler()

#define RX_BUFFER_LEN_MAX 1024 // RX 缓存最大长度

#define recv_view 0 // 回显数据 开启：1

/*******************NB模块配置参数（根据实际参数修改）*****************************/
#define HWY_BROKER_ADDRESS "9f233003dc.st1.iotda-device.cn-north-4.myhuaweicloud.com" //hostname
#define HWY_CLIENTID "686f93ba32771f177b4ab0e9_Car_test_001_0_0_2025071013"
#define HWY_PORT (8883)
#define HWY_USERNAME "686f93ba32771f177b4ab0e9_Car_test_001"
#define HWY_DEVICEID "686f93ba32771f177b4ab0e9_Car_test_001"
#define HWY_PASSWORD "b383446341db9744bc71a740f71f3294a21866a028cccc867ed52d631e1effae"
#define HWY_PUBTOPIC "$oc/devices/HWY_DEVICEID/sys/properties/report"
#define HWY_SUBTOPIC "$oc/devices/HWY_DEVICEID/sys/properties/set/request_id={request_id}"

typedef enum
{
    AT_ERROR = 0,
    AT_OK,
    AT_TIMEOUT
} AT_Rsp_Typedef;

/************************UART API********************************* */
// 串口8初始化
void uart8_init(uint32_t bound);
// 发送1个byte数据
void USART8_send_byte(uint8_t byte);
// 发送多个byte数据
void USART8_send_data(uint8_t *data, uint32_t len);
// 发送字符串
void USART8_send_string(uint8_t *data);

/************************NBIOT API********************************* */
// 模块初始化
void nb_mqtt_init();
// 模块连接服务器
int nb_mqtt_conn(char *brokerAddress, uint16_t port, char *clientID, char *userName, char *password);
// 发布消息
int nb_mqtt_pub(char *topic, char *buf);
// 订阅消息
int nb_mqtt_sub(char *topic);

#endif