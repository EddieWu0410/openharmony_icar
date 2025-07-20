#include "bsp_led.h"

static void led_gpio_config()
{
    /* 开启时钟 */
    crm_periph_clock_enable(LED_CLK, TRUE);
    /* 初始化GPIO */
    gpio_init_type gpio_init_struct;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = LED_PIN;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init(LED_GPIO, &gpio_init_struct);
}

// led初始化
void bsp_led_init(){
    led_gpio_config();
    gpio_bits_reset(LED_GPIO,LED_PIN);
}
// 开灯
void bsp_led_on(){
    gpio_bits_set(LED_GPIO,LED_PIN);
}
// 关灯
void bsp_led_off(){
    gpio_bits_reset(LED_GPIO,LED_PIN); 
}
