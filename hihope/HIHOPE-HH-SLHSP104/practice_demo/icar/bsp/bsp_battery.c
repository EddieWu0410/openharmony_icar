#include "bsp_battery.h"

#include "at32f403a_407_adc.h"

static float battery_get_adcval(void);
static uint8_t battery_get_percentage(void);

Battery_t g_battery = {
    .get_adcval     = battery_get_adcval,
    .get_percentage = battery_get_percentage
};

Battery_t *battery_init(void)
{
    gpio_init_type gpio_initstructure;
    adc_base_config_type adc_base_struct;

    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, AT_TRUE);
    crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK, AT_TRUE);
    crm_adc_clock_div_set(CRM_ADC_DIV_6);

    gpio_default_para_init(&gpio_initstructure);
    gpio_initstructure.gpio_mode = GPIO_MODE_ANALOG;
    gpio_initstructure.gpio_pins = GPIO_PINS_0 ;
    gpio_init(GPIOC, &gpio_initstructure);
    //初始化ADC
    adc_combine_mode_select(ADC_INDEPENDENT_MODE);
    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.sequence_mode = AT_FALSE;
    adc_base_struct.repeat_mode = AT_FALSE;
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 1;
    adc_base_config(ADC1, &adc_base_struct);
    adc_ordinary_conversion_trigger_set(ADC1, ADC12_ORDINARY_TRIG_SOFTWARE, AT_TRUE);
    //基本参数配置结束
    adc_enable(ADC1, AT_TRUE);
    //校准ADC
    adc_calibration_init(ADC1);
    while(adc_calibration_init_status_get(ADC1));
    adc_calibration_start(ADC1);
    while(adc_calibration_status_get(ADC1));

    return &g_battery;
}

void battery_uninit(void)
{
    adc_reset(ADC1);
}

static float battery_get_adcval(void)
{
    adc_ordinary_channel_set(ADC1, ADC_CHANNEL_10, 1, ADC_SAMPLETIME_239_5);

    adc_ordinary_software_trigger_enable(ADC1, AT_TRUE);

    while(!adc_flag_get(ADC1, ADC_CCE_FLAG ));

    return adc_ordinary_conversion_data_get(ADC1)  * 3.3f / 4096 * BATTERY_SCALE_FACTOR;
}

static uint8_t battery_get_percentage(void)
{
    float cur_vol = battery_get_adcval();


    if (cur_vol >= BATTERY_FULL_VOL)
        return 100;

    if (cur_vol < BATTERY_LACK_VOL)
        return 0;

    return (int)(100.0 * (cur_vol - BATTERY_LACK_VOL) / (BATTERY_FULL_VOL - BATTERY_LACK_VOL));

    return 0;
}