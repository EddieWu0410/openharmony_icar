#include "bsp_battery.h"

#include "at32f403a_407_adc.h"
#include <stdio.h>
static float battery_get_info_adcval(void);
static uint8_t battery_get_info_percentage(void);

Battery_t g_battery_info = {
    .get_adcval     = battery_get_info_adcval,
    .get_percentage = battery_get_info_percentage
};

Battery_t *battery_info_init(void)
{
    gpio_init_type gpio_initstructure;
    adc_base_config_type adc_base_struct;

    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, AT_TRUE);
    crm_periph_clock_enable(CRM_ADC1_PERIPH_CLOCK, AT_TRUE);
    crm_adc_clock_div_set(CRM_ADC_DIV_6);

    gpio_default_para_init(&gpio_initstructure);
    gpio_initstructure.gpio_mode = GPIO_MODE_ANALOG;         //模拟量模式
    gpio_initstructure.gpio_pins = GPIO_PINS_0 ;
    gpio_init(GPIOC, &gpio_initstructure);

    adc_combine_mode_select(ADC_INDEPENDENT_MODE);

    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.sequence_mode = AT_FALSE;               //是否扫描
    adc_base_struct.repeat_mode = AT_FALSE;                 //是否循环
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 1;
    adc_base_config(ADC1, &adc_base_struct);

    adc_ordinary_conversion_trigger_set(ADC1, ADC12_ORDINARY_TRIG_SOFTWARE, AT_TRUE);

    adc_enable(ADC1, AT_TRUE);

    adc_calibration_init(ADC1);                           //校准初始化
    while(adc_calibration_init_status_get(ADC1));         //等待初始化完成
    adc_calibration_start(ADC1);                          //开始校准
    while(adc_calibration_status_get(ADC1));              //等待校准完成

    return &g_battery_info;
}

void battery_info_uninit(void)
{
    adc_reset(ADC1);
}

static float battery_get_info_adcval(void)
{
    adc_ordinary_channel_set(ADC1, ADC_CHANNEL_10, 1, ADC_SAMPLETIME_239_5);

    adc_ordinary_software_trigger_enable(ADC1, AT_TRUE);

    while(!adc_flag_get(ADC1, ADC_CCE_FLAG ));

    return adc_ordinary_conversion_data_get(ADC1)  * 3.3f / 4096 * BATTERY_SCALE_FACTOR;
}

static uint8_t battery_get_info_percentage(void)
{
    float cur_vol = battery_get_info_adcval();


    if (cur_vol >= BATTERY_FULL_VOL)
        return 100;

    if (cur_vol < BATTERY_LACK_VOL)
        return 0;

    return (int)(100.0 * (cur_vol - BATTERY_LACK_VOL) / (BATTERY_FULL_VOL - BATTERY_LACK_VOL));

    return 0;
}

uint8_t getBatteryInfo(Battery_t *battery)
{

    
       // Check battery percentage
       uint8_t percentage = battery->get_percentage();
       printf("Battery percentage: %d%%\n", percentage);
    return percentage;
}
