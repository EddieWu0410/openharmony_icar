#include "tracing.h"
#include "../bsp_beep/bsp_beep.h"
uint8_t tracing_state=0;

void set_tracing_state(uint8_t* recv_buffer,uint8_t len)
{
    
    uint8_t state=recv_buffer[4];
    tracing_state=state;
    if(tracing_state){
        // bsp_beep_play(1,20);
        bsp_beep_swich(1,20);
        bsp_beep_swich(2,200);
    }
    else{
        bsp_beep_swich(3,200);
        Motion_Stop(1);
        // bsp_beep_play(200);
        // bsp_beep_play(200);
    }
    
}

uint8_t get_tracing_state()
{

    return tracing_state;
}