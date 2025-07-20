#include "set_car_motion.h"
#include "set_car_run.h"


void set_car_run(uint8_t* recv_buffer,uint8_t len){
    uint8_t state=recv_buffer[5];
    uint8_t speedd=recv_buffer[6];
    uint16_t speed=(uint16_t)speedd*10;
    // uint16_t V_x,V_y,V_z;
    switch (state)
    {
    case 0:
        Motion_Stop(1);
        break;
    
    case 1:
        Motion_Ctrl(speed,0,0,1);
        break;
    case 2:
        Motion_Ctrl(-speed,0,0,1);
        break;
    case 3:
        Motion_Ctrl(0,speed,0,1);
        break;
    case 4:
        Motion_Ctrl(0,-speed,0,1);
        break;
    case 5:
        Motion_Ctrl(0,0,speed,1);
        break;
    case 6:
        Motion_Ctrl(0,0,-speed,1);
        break;
    case 7:
        Motion_Stop(1);
        break;

    default:
        break;
    }


    
    // Motion_Ctrl(V_x,V_y,V_z,1);
    


}