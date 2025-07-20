#ifndef __GAS_H__
#define __GAS_H__

#include "cmsis_os2.h"
#include "ohos_init.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_usart.h"
#include "at32f403a_407_adc.h"

/* 可燃气体传感器功能声明 */
void gas_init(void);        // 传感器初始化函数
uint16_t gas_get_data();    // 获取可燃气体浓度数据函数

#endif // !__GAS_H__