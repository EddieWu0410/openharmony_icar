#ifndef BSP_MOTOR_H
#define BSP_MOTOR_H

#include <stdint.h>
////  tmr2  motor1
////  tmr1  motor2
////  tmr4  motor3
////  tmr9  motor4
// #define MOTOR_SUNRISE_IGNORE_PULSE  (2000)
#define MOTOR_IGNORE_PULSE (1700) 
#define MOTOR_MAX_PULSE (4000)
#define MOTOR_FREQ_DIVIDE (0)

//  左前   左后   右前  右后
//   M1     M2    M2    M4
typedef enum
{
    MOTOR_ID_M1 = 0, // 左前
    MOTOR_ID_M2,     // 左后
    MOTOR_ID_M3,     // 右前
    MOTOR_ID_M4,     // 右后
    MAX_MOTOR
} Motor_ID;

typedef struct
{
    /**
     *@ index 电机索引编号 [0 - 3]
     *@ speed 电机转速 [-100 - 100]
     */
    void (*speed_ctrl)(uint8_t index, int16_t speed);
} Motor_Handler;

void motor_init(void);

void Motor_Set_Pwm(uint8_t id, int16_t speed);
void Motor_Stop(uint8_t brake);
#endif // BSP_MOTOR_H
