#ifndef __TRACING_H_
#define __TRACING_H_

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "../usart1/usart1.h"
#include "../bsp/bsp_motor.h"
#include "../app/app_motion.h"

void set_tracing_state(uint8_t* recv_buffer,uint8_t len);
uint8_t get_tracing_state();

#endif