#include <stdio.h>      // 标准输入输出
#include "ohos_init.h"  // 用于初始化服务(services)和功能(features)
#include "cmsis_os2.h"  // CMSIS-RTOS API V2
#include "wifi_connect.h"

// 设置连接WIFI账号密码
#define WIFI_SSID "openharmony"
#define WIFI_PWD "12345678"

static void *WIFI_TASK(void)
{
    printf(">> wifi Init\n");
    WifiConnect(WIFI_SSID, WIFI_PWD);
    printf("Starting ...\n");
}

static void ExampleEntry(void)
{
    osThreadAttr_t attr;
    attr.name = "WIFI_TASK";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 * 4;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)WIFI_TASK, NULL, &attr) == NULL)
    {
        printf("[GpsExampleEntry] create WIFI_TASK NG\n");
    }
    else
    {
        printf("[GpsExampleEntry] create WIFI_TASK OK\n");
    }
}

APP_FEATURE_INIT(ExampleEntry);