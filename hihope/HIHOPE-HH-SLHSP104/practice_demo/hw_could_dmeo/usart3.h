#ifndef __USART3_H
#define __USART3_H

/*******************************************include***********************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_usart.h"
/*******************************************define***********************************************/
#define Packet_size 128
/*******************************************function***********************************************/

//usart3初始化
void bsp_uart3_init(uint32_t baud);
//串口发送一个字节
void Serial3_SendByte(uint8_t byte);
//串口发送一个数组
void Serial3_SendArray(uint8_t *Array, uint16_t Length);
//串口发送一个字符串
void Serial3_SendString(char *String);
void Serial3_SendNumber(uint32_t Number, uint8_t Length);

#endif