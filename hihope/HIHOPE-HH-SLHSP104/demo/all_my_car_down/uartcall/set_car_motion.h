#ifndef __SET_MOTOR_H_
#define __SET_MOTOR_H_

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "../usart1/usart1.h"
#include "../bsp/bsp_motor.h"
#include "../app/app_motion.h"

void set_car_motion(uint8_t* recv_buffer,uint8_t len);

#endif