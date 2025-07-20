#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

static void *GPS_TASK(void)
{
	while (1)
	{
		parseGpsBuffer();
		printGpsBuffer();
	}
	return NULL;
}

static void GpsExampleEntry(void)
{
	uart7_init(115200);
	clrStruct();
	printf("GPS START!\n");

	osThreadAttr_t attr;
	attr.name = "GPS_TASK";
	attr.attr_bits = 0U;
	attr.cb_mem = NULL;
	attr.cb_size = 0U;
	attr.stack_mem = NULL;
	attr.stack_size = 1024 * 4;
	attr.priority = osPriorityNormal;

	if (osThreadNew((osThreadFunc_t)GPS_TASK, NULL, &attr) == NULL)
	{
		printf("[GpsExampleEntry] create GPS_TASK NG\n");
	}
	else
	{
		printf("[GpsExampleEntry] create GPS_TASK OK\n");
	}
}

APP_FEATURE_INIT(GpsExampleEntry);