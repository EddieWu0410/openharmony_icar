#ifndef __UART1_H_
#define __UART1_H_

#include<stdio.h>
#include<unistd.h>
#include<string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407.h"
#include "at32f403a_407_gpio.h"

void usart1_init(uint32_t bound);
// 发送1个byte数据
void USART7_send_byte(uint8_t byte);
// 发送多个byte数据
void USART7_send_data(uint8_t *data, uint32_t len);
// 发送字符串
void USART7_send_string(uint8_t *data);

extern void Usart1_on_recv(char *data, uint32_t len);

#endif