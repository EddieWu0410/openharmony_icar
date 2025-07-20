#ifndef _UART7_GPS_H_
#define _UART7_GPS_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_usart.h"

#define recv_view 0 //回显接收的数据

//定义数组长度
#define GPS_Buffer_Length 80
#define UTCTime_Length 11
#define latitude_Length 11
#define N_S_Length 2
#define longitude_Length 12
#define E_W_Length 2 

typedef struct SaveData 
{
        char GPS_Buffer[GPS_Buffer_Length];
        char isGetData;                //是否获取到GPS数据
        char isParseData;        //是否解析完成
        char UTCTime[UTCTime_Length];                //UTC时间
        char latitude[latitude_Length];                //纬度
        char N_S[N_S_Length];                //N/S
        char longitude[longitude_Length];                //经度
        char E_W[E_W_Length];                //E/W
        char isUsefull;                //定位信息是否有效
} _SaveData;

extern _SaveData Save_Data;

//uart7初始化
void uart7_init(uint32_t bound);
// 发送1个byte数据
void USART7_send_byte(uint8_t byte);
// 发送多个byte数据
void USART7_send_data(uint8_t* data, uint32_t len);
//发送字符串
void USART7_send_string(uint8_t* data);
//清空串口接收的数据
void CLR_Buf(void);
//识别串口命令
uint8_t Hand(char *a);
//清空结构体数据
void clrStruct(void);

#endif
