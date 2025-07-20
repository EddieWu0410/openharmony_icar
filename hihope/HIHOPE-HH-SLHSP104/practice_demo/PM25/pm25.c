void pm25_init(void)
{
    gpio_init_type gpio_init_struct;
    adc_base_config_type adc_base_struct;
    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_ADC2_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);

    // pm2.5:PA6
    gpio_default_para_init(&gpio_init_struct);
    gpio_init_struct.gpio_mode = GPIO_MODE_ANALOG;
    gpio_init_struct.gpio_pins = GPIO_PINS_6;
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
    