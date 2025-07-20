#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "cmsis_os2.h"
#include "ohos_init.h"
#include "uart8_nb.h"

static void *NB_TASK(void)
{
    uint32_t uwRet = 0;
    int32_t atRet = 0;
    uint32_t count = 0;
    char sendData[1024];
    char *temp = "245";

    printf("[INFO]This is NB_TASK!\r\n");

    nb_mqtt_init();
    /*1.2连接 */
    atRet = nb_mqtt_conn(HWY_BROKER_ADDRESS, HWY_PORT,
                         HWY_CLIENTID, HWY_USERNAME, HWY_PASSWORD);
    while (atRet == AT_OK)
    {
        /*数据sendData */
        memset(sendData, 0, sizeof(sendData));
        sprintf(sendData, "{\"services\":[{\"service_id\":\"%s\",\"properties\":{\"gps_data\":\"%s\"}}]}",
                "car_gps", temp);
        printf("[INFO]*****************************************\r\n");
        printf("[INFO]hwyun send %d-->%s\r\n", count, sendData);
        uwRet = nb_mqtt_pub(HWY_PUBTOPIC, sendData);
        if (uwRet != AT_OK)
            continue;
        osDelay(1000);
        temp = "888";

        if (++count > 2)
            break;
    }
}

static void NbExampleEntry(void)
{
    uart8_init(9600);
    printf("NB START!\n");

    osThreadAttr_t attr;
    attr.name = "NB_TASK";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024 * 4;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)NB_TASK, NULL, &attr) == NULL)
    {
        printf("[GpsExampleEntry] create NB_TASK NG\n");
    }
    else
    {
        printf("[GpsExampleEntry] create NB_TASK OK\n");
    }
}

APP_FEATURE_INIT(NbExampleEntry);