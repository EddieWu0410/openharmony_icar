#include <stdio.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"
#include "usart1/usart1.h"

#include "uartcall/set_motor.h"
#include "uartcall/set_car_motion.h"
#include "uartcall/tracing.h"
#include "uartcall/tracing.h"
// pid和编码器控制的部分
#include "bsp/bsp_motor.h"
#include "bsp/bsp_encoder.h"
#include "bsp/bsp_tracing.h"

#include "bsp_beep/bsp_beep.h"

#include "app/app_motion.h"
#include "app/app_pid.h"
#include "app/app_mecanum.h"
#include "sensor/sensor.h"
#include "sensor/smoker_pm2.5_light.h"
#include "sensor/bme280.h"

#include "uart7_gps/uart7_gps.h"


#include "usart1/protocol.h"
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
        // printf(" [%d] User_main_task_create create %s: NG\n", tid, name);
    }
    else
    {
        // printf(" [%d]User_main_task_create create %s: OK\n", tid, name);
    }
}





static void vTask_Control(void)
{
    while (1)
    {
        // encoder_init();
        // motor_init();
        while (1)
        {
            if (Get_CMD_Flag())
            {
                // Upper_Data_Parse(Get_RxBuffer(), Get_CMD_Length());
                Usart1_on_recv(Get_RxBuffer(), Get_CMD_Length());
                Clear_CMD_Flag();
            }
            osDelay(1);
        }
    }
}



static void TaskSpeed(void)
{
    // 10ms 测速线程
    uint32_t last_tick = osKernelGetTickCount();
    // printf("TaskSpeed \n");
    while (1)
    {
        // osDelay(10);
        last_tick = osKernelGetTickCount();
        osDelayUntil(last_tick + 10);
        Encoder_Update_Count();
        Motion_Handle();
    }
}

static void Send(void)
{
    // uint32_t last_tick = osKernelGetTickCount();
    while(1)
    {
        
        // osDelayUntil(last_tick + 10);
        Send_Encoder_Data();
        osDelay(10);
        // osDelayUntil(last_tick + 10);
        Motion_Send_Data();
        osDelay(10);
        
    }
}




static void Sensor_Get(void)
{

    float DATA1[3]={0};
    float tem_value = 0;
    float hum_value = 0;
    uint32_t ret = 0;
    char payload[2048] = {0};
    while(1)
    {
        memset(payload,0,sizeof(payload));
        get_sensor_public_string(payload);
        printf("payload:\n%s\n", payload);
        Serial3_SendString(payload);
        osDelay(1000);
        // uint32_t light= GET_LIGHT();
        // uint32_t smoke=GET_SMOKER();
        // float gp2= GP2Y1014AU();
        // Get_BME280_Value(DATA1);
        // ret = tem_hum_get_data(&tem_value, &hum_value);
        // if (ret != 0)
        // {
        //     printf("TEM_HUM_TASK ERROR\n");
        // }
        // else
        // {
        //     printf("tem:%.2f hum:%.2f\n", tem_value, hum_value);
        // }

        // printf("light%d\nsmoke:%d\ngp2:%lf\n",light,smoke,gp2);
        // printf("BME:%lf,%lf,%lf",DATA1[0],DATA1[1],DATA1[2]);
    }
}

static void Tracing(void)
{
    int flag=0;
    while(1)
    {
        osDelay(20);
        uint8_t state=get_tracing_state();
        if(state)
        {
            flag=1;
            timer_periodic();
        }
        else
        {   if(flag){
                Motion_Stop(1);
                flag=0;
            }
        }
    }
}

uint16_t rhythm_pattern[] = {
    200, 100,   // 响200ms  静100ms
    200, 100,   // 响200ms  静100ms
    400, 200,   // 响400ms  静200ms
    0             // 末尾以 0 标记结束
};

static void GPS(void)
{
    
    while(1)
    {
        printf("gps_data:\n");
        osDelay(500);
        parseGpsBuffer();
		printGpsBuffer();
    }
}



static void MainEntry(void)
{
    // 初始化UART1，波特率115200
    ctrl_uart1_init();
    motor_init();
    encoder_init();
    PID_Param_Init();
    Trace_init();
    beep_init();

    sensors_init();
    BME280_Init();
    tem_hum_init();
    bsp_uart3_init(115200);

    // uart7_init(115200);
    clrStruct();

    // for (uint32_t i = 0; rhythm_pattern[i] != 0; i += 2) {
    //         // 打开蜂鸣器
    //         bsp_beep_play(rhythm_pattern[i]);
    //         // delay_ms(rhythm_pattern[i]);
    // }
    VTask_Create(TaskSpeed, "TaskSpeed", 1024 * 30, osPriorityLow + 5,NULL); 
    VTask_Create(Send, "Send", 1024 * 20, osPriorityLow + 3,NULL); 
    VTask_Create(Tracing, "Tracing", 1024 * 20, osPriorityLow + 6,NULL); 
    // VTask_Create(GPS, "GPS", 1024 * 20, osPriorityLow + 3,NULL); 
    
    // VTask_Create(Recive_Command, "Recive_Command", 1024 * 30, osPriorityNormal, NULL);
    VTask_Create(Sensor_Get, "Sensor_Get", 1024 * 30, osPriorityLow + 4,NULL);
    VTask_Create(vTask_Control, "vTask_Control", 1024 * 30, osPriorityLow + 8,NULL);
}
APP_FEATURE_INIT(MainEntry);

