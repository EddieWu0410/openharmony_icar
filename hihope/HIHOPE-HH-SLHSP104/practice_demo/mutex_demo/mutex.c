#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#define STACK_SIZE      (2048)
#define DELAY_TICKS_100 (100)
#define TEST_TIMES      (3)
static int times = 0;

// 定义互斥锁对象
osMutexId_t mutex_id;

// 线程a的c函数
void a_c_function() {
    // 获取互斥锁，如果互斥锁不可用则等待
    osMutexAcquire(mutex_id, osWaitForever);
    // 执行c函数的代码
    // ...
    // 释放互斥锁，允许其他线程获取
    printf("b\n");
    usleep(100000);

    osMutexRelease(mutex_id);
}

// 线程b的主函数
void b_main_function() {
    while (1) {
        // 等待某个条件
        // ...
        // 执行b线程的代码
        // ...
        
        printf("a\n");
        usleep(100000);
    }
}

static void TimerTestTask(void)
{
    osThreadAttr_t attr;

    attr.name = "timer_periodic";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = STACK_SIZE;
    attr.priority = 25;

    if (osThreadNew((osThreadFunc_t)a_c_function, NULL, &attr) == NULL) {
        printf("[TimerTestTask] Falied to create timer_periodic!\n");
    }
    
    attr.name = "b_main_function";
    attr.priority = 25;
    if (osThreadNew((osThreadFunc_t)b_main_function, NULL, &attr) == NULL) {
        printf("[TimerTestTask] Falied to create timer_periodic!\n");
    }

    mutex_id = osMutexNew(NULL);
    if (mutex_id == NULL)
    {
         printf("Falied to create Mutex!\n");
    }
}

APP_FEATURE_INIT(TimerTestTask);