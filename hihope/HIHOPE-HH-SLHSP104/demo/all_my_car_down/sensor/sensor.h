#ifndef _SENSOR_H
#define _SENSOR_H
/*******************************************include***********************************************/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "math.h"
#include "smoker_pm2.5_light.h"
#include "cJSON.h"	//cJson解析三方库
#include "../usart3/usart3.h" //usart3串口通信的部分
/*******************************************define***********************************************/

/* 传感器数据类型定义*/
typedef struct
{
    uint8_t illumination; // 百分比
    uint8_t smoke;        // 百分比
    uint8_t pm25;         // 百分比
    // float* BMEDATA;     // 固定大小的数组
	uint8_t temperature;
	uint8_t humidity;
	uint8_t pressure;
	uint8_t longitude;
	uint8_t latitude;
	uint8_t battery;
} SensorData;

// 读取传感器数据，并且将数据值保存到SensorData结构体中
void SensorReadData(SensorData *data);
//获取Json字符串
void get_sensor_public_string(char *payload);
#endif