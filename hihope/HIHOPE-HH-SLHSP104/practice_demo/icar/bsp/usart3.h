#ifndef __USART3_H
#define __USART3_H

#include <stdio.h>
#include <stdint.h>

#include "cmsis_os2.h"

#include "los_task.h"
#include "ohos_init.h"

//不定长hex数据最大数据长度
#define Packet_size 48
extern uint8_t Serial3_data[Packet_size];



// 接收字符的最大值
#define MAX_STRING_LENGTH 512

//数据包数组 大小
// #define Packet_size 8
extern uint8_t Serial3_RxPacket[20];				//定义接收数据包数组

//消息队列的部分
extern osMessageQueueId_t uart3_recv_queue;




//usart3初始化
void ctrl_uart3_init(void);
//串口发送一个字节
void Serial3_SendByte(uint8_t byte);

//串口发送一个数组
void Serial3_SendArray(uint8_t *Array, uint16_t Length);

//串口发送一个字符串
void Serial3_SendString(char *String);

void Serial3_SendNumber(uint32_t Number, uint8_t Length);

#endif
