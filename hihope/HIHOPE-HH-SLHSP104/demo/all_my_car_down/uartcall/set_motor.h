#ifndef __SET_MOTOR_H_
#define __SET_MOTOR_H_

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "../usart1/usart1.h"
#include "../bsp/bsp_motor.h"

void set_motor(char* recv_buffer,uint32_t len);
#endif