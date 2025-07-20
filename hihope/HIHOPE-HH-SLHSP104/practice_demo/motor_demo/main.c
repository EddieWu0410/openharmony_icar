#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

#include "motor.h"


static void *MOTOR_Task(void)
{
    uint16_t speed = 0;
    while (1)
    {
        speed = (++speed)>2000?0:speed;
        Motor_Set_Pwm(0, speed);
        Motor_Set_Pwm(1, speed);
        Motor_Set_Pwm(2, speed);
        Motor_Set_Pwm(3, speed);
        osDelay(50);
    }
}


static void ExampleEntry(void)
{
    /***************motor_init*****************/
    motor_init();

    osThreadAttr_t attr;

    attr.name = "MOTOR_Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 2048;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)MOTOR_Task, NULL, &attr) == NULL)
    {
        printf("[ExampleEntry] create MOTOR_Task NG\n");
    } 
    else
    {
        printf("[ExampleEntry] create MOTOR_Task OK\n");
    }
}
APP_FEATURE_INIT(ExampleEntry);
