#include "light_ill.h"

// 初始化光照传感器（配置GPIO和ADC）
void light_init(void)
{
    gpio_init_type gpio_init_struct;
    adc_base_config_type adc_base_struct;
    
    // 使能GPIOA和ADC2时钟
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_ADC2_PERIPH_CLOCK, TRUE);
    
    // 配置GPIOA5为模拟输入模式
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
    gpio_init_struct.gpio_pins = GPIO_PINS_5;
    gpio_init(GPIOA, &gpio_init_struct);
    
    // 配置ADC时钟（6分频）
    crm_adc_clock_div_set(CRM_ADC_DIV_6);
    
    // ADC基础配置
    adc_combine_mode_select(ADC_INDEPENDENT_MODE); // 独立模式
    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT; // 数据右对齐
    adc_base_struct.ordinary_channel_length = 1;     // 1个转换通道
    adc_base_struct.repeat_mode = AT_FALSE;          // 关闭重复模式
    adc_base_struct.sequence_mode = AT_FALSE;        // 关闭序列模式
    adc_base_config(ADC2, &adc_base_struct);
    
    // 配置软件触发
    adc_ordinary_conversion_trigger_set(ADC2, ADC12_ORDINARY_TRIG_SOFTWARE, AT_TRUE);
    
    // 使能ADC并进行校准
    adc_enable(ADC2, AT_TRUE);
    adc_calibration_init(ADC2);
    while (adc_calibration_init_status_get(ADC2)); // 等待校准初始化
    adc_calibration_start(ADC2);
    while (adc_calibration_status_get(ADC2));      // 等待校准完成
}

// 获取指定ADC通道的采样值（静态函数，内部使用）
static uint16_t get_adc_value(adc_channel_select_type adc_channel)
{
    // 配置ADC通道5（对应GPIOA5）
    adc_ordinary_channel_set(ADC2, adc_channel, 1, ADC_SAMPLETIME_28_5);
    
    // 触发ADC转换
    adc_ordinary_software_trigger_enable(ADC2, AT_TRUE);
    
    // 等待转换完成
    while (!adc_flag_get(ADC2, ADC_CCE_FLAG));
    adc_flag_clear(ADC2, ADC_CCE_FLAG);
    
    // 返回ADC转换结果
    return adc_ordinary_conversion_data_get(ADC2);
}

// 获取光照度数据（主接口）
uint16_t light_get_data()
{
    // 获取ADC通道5的采样值（0-4095）
    uint16_t adc_value = get_adc_value(ADC_CHANNEL_5);
    
    // 将ADC值转换为光照度百分比
    uint32_t Percentage_lux = (adc_value / 4095.0) * 100;
    
    return Percentage_lux;
}