static void *Sensor_TASK(void)
{

    float pm25_value = 0;
    while (1)
    {
        pm25_value=pm25_get_data();
        printf("pm25_value:%.2f\n", pm25_value);   
        osDelay(200);
    }
}

static void MAIN_Entry(void)
{
    /********sensor init*******/
    // PM2.5 init
    pm25_init();

    osThreadAttr_t attr;
    attr.name = "Sensor_TASK";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 * 5;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)Sensor_TASK, NULL, &attr) == NULL)
        {
        printf("[MAIN_Entry] create Sensor_TASK NG\n");
    }
    else
    {
        printf("[MAIN_Entry] create Sensor_TASK OK\n");
    }
}
APP_FEATURE_INIT(MAIN_Entry);