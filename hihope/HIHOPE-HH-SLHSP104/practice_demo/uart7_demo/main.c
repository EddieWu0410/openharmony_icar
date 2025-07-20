#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

#include "uart.h"


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
        printf(" [%p] User_main_task_create create %s: NG\n", tid, name);
    }
    else
    {
        printf(" [%p]User_main_task_create create %s: OK\n", tid, name);
    }
}


#define recv_True 1
#define recv_False 0
#define RECV_BUF_LEN 1024
static uint8_t recv_buffer[RECV_BUF_LEN];
static volatile uint32_t recv_len = 0;
static volatile uint8_t recv_ready = recv_False;

void Usart7_on_recv(char *data, uint32_t len)
{
    if (len > RECV_BUF_LEN)
        len = RECV_BUF_LEN;
    // 按二进制拷贝
    memcpy(recv_buffer, data, len);
    recv_len = len;
    recv_ready = recv_True;
    // 可选：打印十六进制调试
    printf("uart7 recv (len=%lu): ", len);
    for (uint32_t i = 0; i < len; i++)
    {
        printf("%02X ", recv_buffer[i]);
    }
    printf("\n");
}
static void Demo_Task(void)
{
    while (1)
    {
        if (recv_ready)
        {
            // 处理协议头
            if (recv_len >= 2 &&
                (uint8_t)recv_buffer[0] == 0xFF &&
                (uint8_t)recv_buffer[1] == 0xFC)
            {
                printf("Successful analysis\n");
            }
            else
            {
                printf("Header mismatch: %02X %02X\n", recv_buffer[0], recv_buffer[1]);
            }
            // 清理
            memset(recv_buffer, 0, RECV_BUF_LEN);
            recv_ready = recv_False;
            recv_len = 0;
        }
        osDelay(100);
    }
}

static void MainEntry(void)
{
    VTask_Create(Demo_Task, "Demo_Task", 1024 * 4, osPriorityNormal, NULL);
}
APP_FEATURE_INIT(MainEntry);



