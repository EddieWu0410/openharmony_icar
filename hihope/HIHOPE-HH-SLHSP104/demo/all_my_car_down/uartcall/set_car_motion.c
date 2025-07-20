#include "set_car_motion.h"
#include "set_motor.h"


void set_car_motion(uint8_t* recv_buffer,uint8_t len){
    int16_t V_x=recv_buffer[6]<<8 | recv_buffer[5];
    int16_t V_y=recv_buffer[8]<<8 | recv_buffer[7];
    int16_t V_z=recv_buffer[10]<<8 | recv_buffer[9];

    // bsp_beep_play(20);
    // printf("run set car motion\n");
    printf("SET_CAR_MOTION:%d %d %d",V_x,V_y,V_z);
    if(V_x==0&&V_y==0&&V_z==0){
        Motion_Stop(1);
    }
    else{
        Motion_Ctrl(V_x,V_y,V_z,1);
    }


}