// 获传BME280感器数据
float DATA1[3] = {0};
static void *BME280_TASK(void)
{
    while (1)
    {
        Get_BME280_Value(DATA1);
        osDelay(200);
    }
}
static void MAIN_Entry(void)
{
    /********sensor init*******/
    // BME280
    BME280_Init();

    osThreadAttr_t attr;
    attr.name = "BME280_TASK";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
        attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 * 5;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)BME280_TASK, NULL, &attr) == NULL)
    {
        printf("[MAIN_Entry] create BME280_TASK NG\n");
    }
    else
    {
        printf("[MAIN_Entry] create BME280_TASK OK\n");
    }
}
APP_FEATURE_INIT(MAIN_Entry);