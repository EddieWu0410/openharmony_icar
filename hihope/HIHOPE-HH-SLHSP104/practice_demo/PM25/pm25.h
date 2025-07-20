#ifndef __PM25_H__
#define ___PM25_H__ 

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

/* PM2.5 function */
void pm25_init(void);
float pm25_get_data(void);

#endif // !__PM25_H_