#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

#include "bsp_led.h"

static void LED_Task(void)
{
    while (1)
    {
        bsp_led_on();
        printf("led on\n");
        osDelay(500);
        bsp_led_off();
        printf("led off\n");
        osDelay(500);
    }
}

/**
 * 创建任务
 * @param func 任务函数
 * @param name 任务名
 * @param stack_size 堆栈大小
 * @param priority 优先级
 * @param tid_out 任务ID
 */
static void VTask_Create(osThreadFunc_t func, const char *name, uint32_t stack_size, osPriority_t priority, osThreadId_t *tid_out)
{
    osThreadAttr_t attr;
    attr.name = name;
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = stack_size; // 堆栈大小调大一点
    attr.priority = priority;
    osThreadId_t tid = osThreadNew(func, NULL, &attr);
    if (tid_out)
        *tid_out = tid;
    if (tid == NULL)
    {
        printf(" [%d] User_main_task_create create %s: NG\n", tid, name);
    }
    else
    {
        printf(" [%d]User_main_task_create create %s: OK\n", tid, name);
    }
}
static void MainEntry(void)
{
    // bsp init
    bsp_led_init();
    // thread new
    VTask_Create(LED_Task, "LED_Task", 1024, osPriorityNormal, NULL);
}
APP_FEATURE_INIT(MainEntry);