#ifndef BSP_BEEP_H
#define BSP_BEEP_H


#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "at32f403a_407_gpio.h"

void beep_init(void);
void beep_on(void);
void beep_off(void);
void bsp_beep_play(uint16_t time);
void bsp_beep_swich(uint8_t count, uint16_t time);

#endif  // BSP_BEEP_H