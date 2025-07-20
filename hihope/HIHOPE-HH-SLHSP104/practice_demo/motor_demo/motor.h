#ifndef __MOTOR_H_
#define __MOTOR_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407.h"
#include "at32f403a_407_gpio.h"



#define MOTOR_IGNORE_PULSE  (2000)
#define MOTOR_MAX_PULSE     (4000)
#define MOTOR_FREQ_DIVIDE   (0)

typedef enum {
    MOTOR_ID_M1 = 0,
    MOTOR_ID_M2,
    MOTOR_ID_M3,
    MOTOR_ID_M4,
    MAX_MOTOR
} Motor_ID;

typedef struct {
    /**
    *@ index 电机索引编号 [0 - 3]
    *@ speed 电机转速 [-100 - 100]
    */
    void (*speed_ctrl)(uint8_t index, int16_t speed);
}Motor_Handler;

/******************function**************/
void motor_init(void);
void Motor_Set_Pwm(uint8_t id, int16_t speed);
void Motor_Stop(uint8_t brake);

#endif
