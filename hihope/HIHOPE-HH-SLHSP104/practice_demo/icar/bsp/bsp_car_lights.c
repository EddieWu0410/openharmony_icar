#include "bsp_car_lights.h"


void car_light_power_on(void);

void car_light_init(void)
{

    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_TMR2_PERIPH_CLOCK, TRUE);

    gpio_init_type gpio_config;
    tmr_output_config_type tmr_oc_init_structure;
    gpio_config.gpio_pins = GPIO_PINS_10 | GPIO_PINS_11;
    gpio_config.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_config.gpio_pull = GPIO_PULL_NONE;
    gpio_config.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_config.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init(GPIOB, &gpio_config);

    car_light_power_on();
}

void set_on_left_light(void)
{
    gpio_bits_set(GPIOB, GPIO_PINS_10);
}
void set_on_right_light(void)
{
    gpio_bits_set(GPIOB, GPIO_PINS_11);
}
void set_off_left_light(void)
{
    gpio_bits_reset(GPIOB, GPIO_PINS_10);
}
void set_off_right_light(void)
{
    gpio_bits_reset(GPIOB, GPIO_PINS_11);
}

void car_light_power_on(void)
{
    set_on_left_light();
    set_on_right_light();
    osDelay(5000);
    set_off_left_light();
    set_off_right_light();
}