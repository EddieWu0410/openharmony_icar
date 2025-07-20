#include <stdio.h>
#include "cmsis_os2.h"
#include "los_task.h"
#include "ohos_init.h"
#include "at32f403a_407.h"
#include "cJSON.h" //cJson解析三方库
#include "usart3.h" //usart3串口通信的部分
#include "smoker_pm2.5_light.h" //烟雾、pm25、光照
#include "sensor.h" //命令函数的部分

/*********************************************上传数据***********************************************************/
void at32_send_task(void)
{
   char payload[2048] = {0};
   while (1)
   {
      get_sensor_public_string(payload);
      printf("payload:%s\n", payload);
      Serial3_SendString(payload);
      osDelay(1000);
   }
}

/*********************************************创建线程***********************************************************/
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

/**********************************************主函数入口**********************************************************/
void User_main_task_create(void)
{
   // 初始化uart3串口
   bsp_uart3_init(115200);
   sensors_init();
   VTask_Create(at32_send_task, "at32_send_task", 1024 * 10, osPriorityNormal, NULL);
}

APP_FEATURE_INIT(User_main_task_create);