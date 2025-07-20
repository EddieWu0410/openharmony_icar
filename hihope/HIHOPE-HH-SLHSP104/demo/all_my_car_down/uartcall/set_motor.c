#include "set_motor.h"

static void Motion_Set_Pwn(int16_t speed1,int16_t speed2,int16_t speed3,int16_t speed4){

    Motor_Set_Pwm(0,speed1);
    Motor_Set_Pwm(1,speed2);
    Motor_Set_Pwm(2,speed3);
    Motor_Set_Pwm(3,speed4);
    return;
}

void set_motor(char *recv_buffer,uint32_t len){  
    int8_t m1_speed = (int8_t)recv_buffer[4];  // M1电机速度
    int8_t m2_speed = (int8_t)recv_buffer[5];  // M2电机速度
    int8_t m3_speed = (int8_t)recv_buffer[6];  // M3电机速度
    int8_t m4_speed = (int8_t)recv_buffer[7];  // M4电机速度

    int16_t speed1=(int16_t)m1_speed*((MOTOR_MAX_PULSE - MOTOR_IGNORE_PULSE)/100.0);
    int16_t speed2=(int16_t)m2_speed*((MOTOR_MAX_PULSE - MOTOR_IGNORE_PULSE)/100.0);
    int16_t speed3=(int16_t)m3_speed*((MOTOR_MAX_PULSE - MOTOR_IGNORE_PULSE)/100.0);
    int16_t speed4=(int16_t)m4_speed*((MOTOR_MAX_PULSE - MOTOR_IGNORE_PULSE)/100.0);
    Motion_Set_Pwn(speed1,speed2,speed3,speed4);
}

