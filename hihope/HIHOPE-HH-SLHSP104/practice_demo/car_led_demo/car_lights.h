#ifndef _CAR_LIGHTS_H
#define _CAR_LIGHTS_H

/*******************************************include***********************************************/
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "at32f403a_407_gpio.h"   /* 驱动层 GPIO 定义 */
#include "at32f403a_407_crm.h"    /* 时钟使能 */
#include "cmsis_os2.h"      
/*******************************************define***********************************************/

/*******************************************function***********************************************/
void car_light_init(void);
void set_on_left_light(void);
void set_on_right_light(void);
void set_off_left_light(void);
void set_off_right_light(void);
void car_light_power_on(void);

#endif