#ifndef BSP_BATTERY_H
#define BSP_BATTERY_H


#include <stdint.h>

#define BATTERY_FULL_VOL          12.6f
#define BATTERY_LACK_VOL          10.5f

#define BATTERY_SCALE_FACTOR     (13.3f / 3.3f)

typedef struct {
    float (*get_adcval)(void);
    uint8_t (*get_percentage)(void);
}Battery_t;

Battery_t *battery_init(void);
void battery_uninit(void);

#endif  // BSP_BATTERY_H