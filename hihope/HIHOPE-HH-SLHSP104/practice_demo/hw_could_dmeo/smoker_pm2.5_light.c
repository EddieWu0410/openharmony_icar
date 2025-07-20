#include "smoker_pm2.5_light.h"

// float AD_PM;

void sensors_init(void)
{
    gpio_init_type gpio_init_struct;
    adc_base_config_type adc_base_struct;
    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_ADC2_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);
    // smoker:PA4  pm2.5:PA6  light:PA5
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
    gpio_init_struct.gpio_pins = GPIO_PINS_4 | GPIO_PINS_5 | GPIO_PINS_6;
    gpio_init(GPIOA, &gpio_init_struct);
    crm_adc_clock_div_set(CRM_ADC_DIV_6);
    // pm2.5led:PC8
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pins = GPIO_PINS_8;
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;
    gpio_init(GPIOC, &gpio_init_struct);
    gpio_bits_set(GPIOC, GPIO_PINS_8);
    // adc配置
    adc_combine_mode_select(ADC_INDEPENDENT_MODE);
    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 1;
    adc_base_struct.repeat_mode = AT_FALSE;
    adc_base_struct.sequence_mode = AT_TRUE;
    adc_base_config(ADC2, &adc_base_struct);

    adc_ordinary_conversion_trigger_set(ADC2, ADC12_ORDINARY_TRIG_SOFTWARE, AT_TRUE);
    adc_enable(ADC2, AT_TRUE);

    adc_calibration_init(ADC2);
    while (adc_calibration_init_status_get(ADC2))
        ;
    adc_calibration_start(ADC2);
    while (adc_calibration_status_get(ADC2))
        ;
}

uint16_t get_sensors_value(adc_channel_select_type adc_channel)
{
    // adc_ordinary_channel_set(ADC2, adc_channel, 1, ADC_SAMPLETIME_239_5);
    adc_ordinary_channel_set(ADC2, adc_channel, 1, ADC_SAMPLETIME_28_5);
    adc_ordinary_software_trigger_enable(ADC2, AT_TRUE);
    while (!adc_flag_get(ADC2, ADC_CCE_FLAG))
        ;
    adc_flag_clear(ADC2, ADC_CCE_FLAG);
    return adc_ordinary_conversion_data_get(ADC2);
}

void set_led_high(void)
{

    GPIOC->odt |= GPIO_PINS_8;
}
void set_led_low(void)
{
    GPIOC->odt &= ~GPIO_PINS_8;
}

float conver_adc_to_v(uint16_t temp)
{
    return temp * 3.3 / 4096;
}
/**
 * 获取PM2.5的ADC值并转换
 */
float GP2Y1014AU(void)
{
    uint16_t adc_value = 0;
    set_led_high();
    set_led_low();
    dwt_delay_us(280);
    adc_value = get_sensors_value(ADC_CHANNEL_6);
    dwt_delay_us(23);
    set_led_high();
    set_led_low();
    dwt_delay_us(9680);
    float Vtemp = conver_adc_to_v(adc_value);
    // AD_PM = 0.17 * Vtemp - 0.1; // 转换公式
    // return AD_PM;
    return (adc_value / 4095.0) * 100; // percent
    // return 0.17 * Vtemp - 0.1;
}
/**
 * 获取可燃气体的ADC值并转换
 */
uint32_t GET_SMOKER(void)
{
    uint16_t adc_value = get_sensors_value(ADC_CHANNEL_4);
    uint32_t Percentage_smoker = (1-(adc_value / 4095.0)) * 100;
    // return adc_value;
    return Percentage_smoker;
}

/**
 * 获取光照度的ADC值并转换
 */
uint32_t GET_LIGHT(void)
{
    uint16_t adc_value = get_sensors_value(ADC_CHANNEL_5);
    uint32_t Percentage_lux = (1-(adc_value / 4095.0)) * 100;
    return Percentage_lux;
    // return adc_value;
}

uint16_t Get_Adc_Average(uint8_t times, adc_channel_select_type adc_channel)
{
    uint32_t temp_val = 0;
    uint8_t t;
    for (t = 0; t < times; t++)
    {
        temp_val += get_sensors_value(adc_channel);
        delay_ms(5);
    }
    return temp_val / times;
}