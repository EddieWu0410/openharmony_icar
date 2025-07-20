#include <stdio.h>
#include <unistd.h>
#include "tem_hum.h"
#include "cmsis_os2.h"
#include "ohos_init.h"

// 获取温湿度的值
static void *TEM_HUM_TASK(void)
{

    float tem_value = 0;
    float hum_value = 0;
    uint32_t ret = 0;
    while (1)
    {
        ret = tem_hum_get_data(&tem_value, &hum_value);
        if (ret != 0)
        {
            printf("TEM_HUM_TASK ERROR\n");
        }
        else
        {
            printf("tem:%.2f hum:%.2f\n", tem_value, hum_value);
        }
        osDelay(200);
    }
}

static void MAIN_Entry(void)
{
    /********sensor init*******/
    // Tem&Hum
    tem_hum_init();

    osThreadAttr_t attr;
    attr.name = "TEM_HUM_TASK";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 * 5;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)TEM_HUM_TASK, NULL, &attr) == NULL)
    {
        printf("[MAIN_Entry] create TEM_HUM_TASK NG\n");
    }
    else
    {
        printf("[MAIN_Entry] create TEM_HUM_TASK OK\n");
    }
}

APP_FEATURE_INIT(MAIN_Entry);