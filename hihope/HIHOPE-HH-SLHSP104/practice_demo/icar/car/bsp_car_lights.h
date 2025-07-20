#ifndef _CAR_LIGHTS_H
#define _CAR_LIGHTS_H

#include "at32f403a_407.h"
#include "at32f403a_407_gpio.h"
#include "cmsis_os2.h"

void car_light_init(void);
void set_on_left_light(void);
void set_on_right_light(void);
void set_off_left_light(void);
void set_off_right_light(void);

void car_light_power_on(void);

#endif