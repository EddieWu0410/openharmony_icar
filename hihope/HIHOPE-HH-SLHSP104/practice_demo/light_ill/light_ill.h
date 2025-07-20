#ifndef __LIGHT_ILL_H__
#define __LIGHT_ILL_H__

#include "cmsis_os2.h"
#include "ohos_init.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "at32f403a_407_crm.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_usart.h"
#include "at32f403a_407_adc.h"

/* ligth   function */
void light_init(void);   // light init
uint16_t light_get_data();    // get light data

#endif // !__LIGHT_H__