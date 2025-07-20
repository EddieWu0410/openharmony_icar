void light_init(void)
{
    gpio_init_type gpio_init_struct;
    adc_base_config_type adc_base_struct;
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
        crm_periph_clock_enable(CRM_ADC2_PERIPH_CLOCK, TRUE);
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
    gpio_init_struct.gpio_pins = GPIO_PINS_5;
    gpio_init(GPIOA, &gpio_init_struct);
    crm_adc_clock_div_set(CRM_ADC_DIV_6);

    // adc_config
    adc_combine_mode_select(ADC_INDEPENDENT_MODE);
    adc_base_default_para_init(&adc_base_struct);
    adc_base_struct.data_align = ADC_RIGHT_ALIGNMENT;
    adc_base_struct.ordinary_channel_length = 1;
    adc_base_struct.repeat_mode = AT_FALSE;
    adc_base_struct.sequence_mode = AT_FALSE;
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

static uint16_t get_adc_value(adc_channel_select_type adc_channel)
{
    adc_ordinary_channel_set(ADC2, adc_channel, 1, ADC_SAMPLETIME_28_5);
        adc_ordinary_software_trigger_enable(ADC2, AT_TRUE);
    while (!adc_flag_get(ADC2, ADC_CCE_FLAG))
        ;
    adc_flag_clear(ADC2, ADC_CCE_FLAG);
    return adc_ordinary_conversion_data_get(ADC2);
}

uint16_t light_get_data()
{
    uint16_t adc_value = get_adc_value(ADC_CHANNEL_5);
    // 比例法，根据电阻值得出光照度
    uint32_t Percentage_lux = (adc_value/4095.0)*100;
    return Percentage_lux;
}