#ifndef _SMOKER_PM_LIGHT_H
#define _SMOKER_PM_LIGHT_H

#include <stdint.h>
#include "at32f403a_407_adc.h"
#include "at32f403a_407_clock.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_board.h"
#include "cmsis_os2.h"

//传感器初始化
void  sensors_init(void);
//获取传感器的adc值
uint16_t get_sensors_value(adc_channel_select_type adc_channel);
//获取PM2.5的值
float GP2Y1014AU(void);
//获取可燃气体传感器的值
uint32_t GET_SMOKER(void);
//获取光照度传感器的值
uint32_t GET_LIGHT(void);
//获取ADC平均值
uint16_t Get_Adc_Average(uint8_t times,adc_channel_select_type adc_channel);
//ADC电压值转换
float conver_adc_to_v(uint16_t temp);

#endif