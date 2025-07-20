#ifndef __BEEPING_H_
#define __BEEPING_H_

#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "../usart1/usart1.h"
#include "../bsp/bsp_motor.h"
#include "../bsp_beep/bsp_beep.h"
#include "../app/app_motion.h"

void set_beeping(uint8_t* recv_buffer,uint8_t len);

#endif