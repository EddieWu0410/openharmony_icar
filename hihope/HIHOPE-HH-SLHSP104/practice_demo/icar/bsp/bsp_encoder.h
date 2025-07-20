#ifndef BSP_ENCODER_H
#define BSP_ENCODER_H

#define YABO_CAR    0
#define RUNHE_CAR   1
#include <stdint.h>
#include "bsp_motor.h"
typedef struct {
    int32_t l1_cnt;
    int32_t l2_cnt;
    int32_t l3_cnt;
    int32_t l4_cnt;
}Encoder_Cnt_t;

void encoder_uninit(void);
void encoder_init(void);
void Encoder_Update_Count(void);
void Encoder_Get_ALL(int* Encoder_all);
void Send_Encoder_Data(void);

#endif  // ENCODER_H
