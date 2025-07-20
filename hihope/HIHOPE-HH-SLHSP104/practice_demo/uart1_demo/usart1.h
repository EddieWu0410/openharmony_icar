#ifndef __USART1_H
#define __USART1_H

#include <stdio.h>
#include <stdint.h>

#include "cmsis_os2.h"

#include "los_task.h"
#include "ohos_init.h"
#include "protocol.h"

//不定长hex数据最大数据长度
#define Packet_size 512
extern uint8_t Serial1_data[Packet_size];
extern uint8_t Serial1_RxPacket[Packet_size];				//定义接收数据包数组


// 接收字符的最大值
#define MAX_STRING_LENGTH 512

//数据包数组 大小
// #define Packet_size 8


//消息队列的部分
extern osMessageQueueId_t uart1_recv_queue;




//usart1初始化
void ctrl_uart1_init(void);
//串口发送一个字节
void Serial1_SendByte(uint8_t byte);

//串口发送一个数组
void Serial1_SendArray(uint8_t *Array, uint16_t Length);

//初始化DMA
void at32_dma_init(void);
//DMA 发送 接受
// void usartdmarecv(u8 *data,u16 len);
// void usartdmasend(u8 *buf, u16 len);
void usartdmasend(uint8_t *buf, uint16_t len);
//串口发送一个字符串
void Serial1_SendString(char *String);

void Serial1_SendNumber(uint32_t Number, uint8_t Length);
// extern void Usart1_on_recv(char *data,uint32_t len);
#endif
