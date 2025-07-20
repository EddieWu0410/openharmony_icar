#ifndef BSP_PID_H
#define BSP_PID_H

#include <stdint.h>

#define PID_CNT 4

#define bsp_Kp  1.5f
#define bsp_Ki  0.1f
#define bsp_Kd  8.0f

typedef void    (*set_val_func)(uint8_t index, int16_t val);
typedef int16_t (*get_val_func)(uint8_t index, uint8_t is_clean);

typedef struct {
    void (*set_target_val)(int index, int val);
}PID_Cal_t;

PID_Cal_t *pid_init(set_val_func set_val, get_val_func get_val);
void pid_uninit(void);

#endif  // BSP_PID_H
