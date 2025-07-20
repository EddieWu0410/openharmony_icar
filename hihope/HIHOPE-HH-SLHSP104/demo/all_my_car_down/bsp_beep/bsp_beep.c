#include "bsp_beep.h"


void beep_init(void)
{
    // GPIO_PC1=[GPIOC, GPIO_PINS_1] 控制开发板上的led灯

    // 开启GPIOC的时钟
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);

    // gpio结构体
    gpio_init_type gpio_config;

    // io管脚
    gpio_config.gpio_pins = GPIO_PINS_2;

    // 输出模式
    gpio_config.gpio_mode = GPIO_MODE_OUTPUT;

    // 无上下拉电阻
    gpio_config.gpio_pull = GPIO_PULL_NONE;

    // 设置为推挽输出模式,即引脚可以提供高电平和低电平输出。
    gpio_config.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;

    /*
    GPIO_DRIVE_STRENGTH_MODERATE 对应适中的电流推动/吸入能力
    GPIO_DRIVE_STRENGTH_STRONGER 对应较大的电流推动/吸入能力
    GPIO_DRIVE_STRENGTH_MAXIMUM 对应极大的电流推动/吸入能力
    如果 IO 速度设置为最大的推动力设置，且负载较小时，易在 IO 上产生过冲振铃现象，存在影响应用的可能性
    */
    gpio_config.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;

    // 初始化gpio
    gpio_init(GPIOC, &gpio_config); // group C

    return NULL;
}

void beep_on(void)
{
    GPIOC->odt |= GPIO_PINS_2;
}

void beep_off(void)
{
    GPIOC->odt &= ~GPIO_PINS_2;
}
// 0:关闭    1:开启    >=10:延迟time ms自动关闭
void bsp_beep_play(uint16_t time)
{
    if (time == 1)
    {
        beep_on();
    }
    else if (time == 0)
    {
        beep_off();
    }
    else
    {
        if (time >= 10)
        {
            beep_on();
            osDelay(time);
            beep_off();
        }
    }
}
// 控制蜂鸣器次数和时间
void bsp_beep_swich(uint8_t count, uint16_t time)
{
    for (uint8_t i = 0; i < count; i++)
    {
        beep_on();
        osDelay(time);
        beep_off();
        osDelay(time);
    }
}