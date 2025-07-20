#include "beeping.h"

void set_beeping(uint8_t* recv_buffer,uint8_t len)
{
    uint8_t lower=recv_buffer[4];
    uint8_t higher=recv_buffer[5];
    uint16_t time= (uint16_t)(higher << 8) +(uint16_t) lower;
    bsp_beep_play(time);
    return;
}