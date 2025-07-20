#ifndef  _BSP_TRACING_H
#define  _BSP_TRACING_H

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "at32f403a_407.h"
#include "at32f403a_407_gpio.h"
#include "bsp_motor.h"
#include "bsp_encoder.h"
#include "app_mecanum.h"

extern osThreadId_t trcae_tid;

int timer_periodic(void);
void Trace_init();
void Trace_Ctrl(uint8_t ctrl_flag);

#endif