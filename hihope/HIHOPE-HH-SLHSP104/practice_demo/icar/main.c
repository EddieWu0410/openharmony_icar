#include <stdio.h>
#include "cmsis_os2.h"
#include "los_task.h"
#include "ohos_init.h"
#include "at32f403a_407.h"

#include "cJSON.h"  //cJson解析三方库
#include "usart3.h" //usart3串口通信的部分
#include "usart1.h" //usart1串口通信的部分

// pid和编码器控制的部分
#include "bsp_motor.h"
#include "bsp_encoder.h"
#include "bsp_beep.h"
#include "bsp_tracing.h"
#include "protocol.h"

#include "battery_info.h" //电池电量
#include <string.h>
#include "app_motion.h"
#include "app_pid.h"

#include "bsp_car_lights.h"
#include "bsp_mpu9250.h"
#include "app_mecanum.h"
#include "app_angle.h"
// #include "bsp_timer14.h"
/********************************************************************************************************/

/********************************************************************************************************/
/********************************************************************************************************/

/********************************************************************************************************/
void bsptask(void)
{
    printf("motor start  \n");
    encoder_init();
    motor_init();

    int Encoder_All_Now[MAX_MOTOR] = {0};
    // Motor_Set_Pwm(0, 2); // 2500
    // Motor_Set_Pwm(1, 2);
    // Motor_Set_Pwm(2, 2);
    // Motor_Set_Pwm(3, 2);
    while (1)
    {
        osDelay(1000);
        // printf("acc XYZ:|	%d	|	%d	|	%d	|\n", g_imu_data.accX, g_imu_data.accY, g_imu_data.accZ);
        // printf("gyro XYZ:|	%d	|	%d	|	%d	|\n", g_imu_data.gyroX, g_imu_data.gyroY, g_imu_data.gyroZ);
        Encoder_Update_Count();
        Encoder_Get_ALL(Encoder_All_Now);
        // M1 M2 M3 M4
        printf("1:%d,2:%d,3:%d,4:%d  \n", Encoder_All_Now[0], Encoder_All_Now[1], Encoder_All_Now[2], Encoder_All_Now[3]);
    }
}
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
/* 解析上位机发送过来的命令, 并处理控制的内容 */
void vTask_Control(void)
{
    encoder_init();
    motor_init();
    while (1)
    {
        if (Get_CMD_Flag())
        {
            Upper_Data_Parse(Get_RxBuffer(), Get_CMD_Length());
            Clear_CMD_Flag();
        }
        osDelay(1);
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
// vTask_Auto_Report task
void vTask_Auto_Report(void)
{
    uint8_t report_count = 0;
    while (1)
    {
        if (report_count == 0)
        {
            Motion_Send_Data();
        }
        else if (report_count == 11)
        {
            Get_mpu9250_value();
        }
        else if (report_count == 21)
        {
            Get_Roll_Pitch();
        }
        else if (report_count == 31)
        {
            Send_Encoder_Data();
        }
        // printf("report_count:%d\r\n", report_count);
        report_count++;
        if (report_count >= 40)
            report_count = 0;
        osDelay(1); // 任务每10ms判断一次
    }
}
void vTask_IMU()
{
    uint32_t last_tick = osKernelGetTickCount();
    while (1)
    {
        MPU9250_Read_Data_Handle();
        // printf("imu.yaw:%f,%f,%f\r\n", imu.roll, imu.pitch, imu.yaw);
        //    printf("%.3f,  %.3f,  %.3f,  %.3f,   %.3f,  %.3f\r\n", acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z);
        // printf("%f,  %f, %f ,%f, %f, %f, %f\r\n", (float)(gyro[2]), _gyro[2], gyro_z, imu.yaw, imu.acc_g.z, imu.roll, imu.pitch);
        last_tick = osKernelGetTickCount();
        osDelayUntil(last_tick + 10); // 根据IMU采样频率
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

    /****************************************************/
    // 初始化uart3串口
    ctrl_uart3_init();
    ctrl_uart1_init();
    /****************************************************/

    /****************************************************/
    // 初始化板载蜂鸣器
    beep_init();
    /****************************************************/
    // pid 参数初始化
    PID_Param_Init();
    // MPU_9250 init
    MPU9250_Init();

    // bsp_timer14_init();
    // soft_timer1_init();
    // 车灯初始化
    car_light_init();

    // VTask_Create(bsptask,"bsptask",1024*5,osPriorityLow);
    VTask_Create(vTask_Trace, "vTask_Trace", 1024 * 20, osPriorityLow+2,&trcae_tid);
    osThreadSuspend(trcae_tid);
    VTask_Create(TaskSpeed, "TaskSpeed", 1024 * 20, osPriorityLow + 8,NULL);                 // 6    + 10
    VTask_Create(vTask_Control, "vTask_Control", 1024 * 20, osPriorityLow + 7,NULL);         // 5    + 9
    VTask_Create(vTask_Auto_Report, "vTask_Auto_Report", 1024 * 20, osPriorityLow + 4,NULL); // 3    +7
    VTask_Create(vTask_IMU, "vTask_IMU", 1024 * 20, osPriorityLow,NULL);                     // 1    +3
}
APP_FEATURE_INIT(User_main_task_create);