#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "los_task.h"
#include "ohos_init.h"
#include "at32f403a_407.h"


// pid和编码器控制的部分
#include "bsp_motor.h"
#include "bsp_encoder.h"
#include "bsp_tracing.h"

#include "app_motion.h"
#include "app_pid.h"
#include "app_mecanum.h"
/********************************************************************************************************/

/********************************************************************************************************/

void TaskSpeed(void)
{
    // 10ms 测速线程
    uint32_t last_tick = osKernelGetTickCount();
    printf("TaskSpeed \n");
    while (1)
    {
        // osDelay(10);
        last_tick = osKernelGetTickCount();
        osDelayUntil(last_tick + 10);
        Encoder_Update_Count();
        Motion_Handle();
    }
}

// 循迹线程
osThreadId_t trcae_tid;
void vTask_Trace(void)
{

    printf("start trace\n");
    while (1)
    {
        timer_periodic();
        osDelay(20);
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
void User_main_task_create(void)
{
    /****************************************************/
    //电机初始化
    motor_init();
    //编码器初始化
    encoder_init();
    // pid 参数初始化
    PID_Param_Init();
    //循迹模块初始化
    Trace_init();
    /****************************************************/
    VTask_Create(vTask_Trace, "vTask_Trace", 1024 * 20, osPriorityLow+2,&trcae_tid);
    // osThreadSuspend(trcae_tid);
    VTask_Create(TaskSpeed, "TaskSpeed", 1024 * 20, osPriorityLow + 8,NULL);                 // 6
}
APP_FEATURE_INIT(User_main_task_create);
