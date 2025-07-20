/*******************************************define***********************************************/
#ifndef __UART_H_
#define __UART_H_
#include <stdint.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407.h"
#include "at32f403a_407_gpio.h"

/*******************************************define***********************************************/
#define USART7_RECV_CALLBACK 1 // 1开启回调函数
/*******************************************function***********************************************/
// uart7初始化
void uart7_init(uint32_t bound);
// 发送1个byte数据
void USART7_send_byte(uint8_t byte);
// 发送多个byte数据
void USART7_send_data(uint8_t *data, uint32_t len);
// 发送字符串
void USART7_send_string(uint8_t *data);
#if USART7_RECV_CALLBACK                              // 需要自己实现  void Usart7_on_recv(char *data,uint32_t len){ printf("recv: %s\r\n", data);}
extern void Usart7_on_recv(char *data, uint32_t len); // 回调函数   处理接收到的数据
#endif
#endif
