static uint16_t get_adc_value(adc_channel_select_type adc_channel)
{
    adc_ordinary_channel_set(ADC2, adc_channel, 1, ADC_SAMPLETIME_28_5);
    adc_ordinary_software_trigger_enable(ADC2, AT_TRUE);
    while (!adc_flag_get(ADC2, ADC_CCE_FLAG))
        ;
    adc_flag_clear(ADC2, ADC_CCE_FLAG);
    return adc_ordinary_conversion_data_get(ADC2);
}
// 获取pm2.5数据
float pm25_get_data(void)
{
    uint16_t adc_value = 0;

    gpio_bits_reset(GPIOC, GPIO_PINS_8);
    usleep(280);
    adc_value = get_sensors_value(ADC_CHANNEL_6);
    usleep(23);
    gpio_bits_set(GPIOC, GPIO_PINS_8);
    usleep(9680);
    return 0.17 * adc_value - 0.1; // 单位：ug/m3
}