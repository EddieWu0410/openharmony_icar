#ifndef __BSP_LED_H__
#define __BSP_LED_H__

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407.h"
#include "at32f403a_407_gpio.h"

/*******************************************define***********************************************/
#define LED_CLK CRM_GPIOC_PERIPH_CLOCK 
#define LED_PIN GPIO_PINS_1
#define LED_GPIO GPIOC 
/*******************************************funcion***********************************************/
//led初始化
void bsp_led_init();
//开灯
void bsp_led_on();
//关灯
void bsp_led_off();


#endif