#ifndef __BSP_TIMER14_H__
#define __BSP_TIMER14_H__

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_tmr.h"
#include "bsp_mpu9250.h"
// #include "zf_device_imu660ra.h"


//用于接解析姿态角
void bsp_timer14_init(void);



#endif