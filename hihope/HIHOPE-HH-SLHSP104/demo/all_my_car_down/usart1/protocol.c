#include <stdio.h>
#include <string.h>
#include "cmsis_os2.h"
#include "los_task.h"
#include "ohos_init.h"
#include "at32f403a_407.h"
// #include "cJSON.h"  //cJson解析三方库
// #include "usart3.h" //usart3串口通信的部分
#include "usart1.h" //usart1串口通信的部分

// pid和编码器控制的部分
#include "bsp_motor.h"
#include "bsp_encoder.h"
#include "bsp_beep.h"
#include "bsp_tracing.h"
#include "protocol.h"
//
#include "app_motion.h"
#include "app_pid.h"
#include "app_mecanum.h"
#include "protocol.h"
#include "../uartcall/set_car_motion.h"
#include "../uartcall/tracing.h"
#include "../uartcall/beeping.h"

/********************************************************************************************************/
/* 命令接收缓存 */
uint8_t RxBuffer[PTO_MAX_BUF_LEN];
/* 接收数据下标 */
uint8_t RxIndex = 0;
/* 接收状态机 */
uint8_t RxFlag = 0;
/* 新命令接收标志 */
uint8_t New_CMD_flag = 0;
/* 新命令数据长度 */
uint8_t New_CMD_length;
/********************************************************************************************************/
// 获取命令标志
uint8_t Get_CMD_Flag(void)
{
    return New_CMD_flag;
}
// 获取接收的数据
uint8_t *Get_RxBuffer(void)
{
    return (uint8_t *)RxBuffer;
}
// 获取命令长度
uint8_t Get_CMD_Length(void)
{
    return New_CMD_length;
}
// 清除命令数据和相关标志
void Clear_CMD_Flag(void)
{
    for (uint8_t i = 0; i < New_CMD_length; i++)
    {
        RxBuffer[i] = 0;
    }
    New_CMD_length = 0;
    New_CMD_flag = 0;
}
/********************************************************************************************************/
// 接收数据
void Upper_Data_Receive(uint8_t Rx_Temp)
{
    // printf("recieve_data:%#2x\n",Rx_Temp);
    if (New_CMD_flag != 0)
        return;
    switch (RxFlag)
    {
    case 0:
        if (Rx_Temp == PTO_HEAD)
        {
            RxBuffer[0] = PTO_HEAD;
            // printf("PTO_HEAD:%#2x   ", RxBuffer[0]);
            RxFlag = 1;
        }
        else
        {
            RxBuffer[0] = Rx_Temp;
            // printf("PTO_HEAD ERR:%#2x\n", RxBuffer[0]);
            RxBuffer[0] = 0x0;
            RxFlag = 0;
        }
        break;

    case 1:
        if (Rx_Temp == PTO_DEVICE_ID)
        {
            RxBuffer[1] = PTO_DEVICE_ID;
            // printf("PTO_DEVICE_ID:%#2x  ", RxBuffer[1]);
            RxFlag = 2;
            RxIndex = 2;
        }
        else
        {
            RxBuffer[1] = Rx_Temp;
            // printf("PTO_DEVICE_ID ERR:%#2x\n", RxBuffer[1]);
            RxFlag = 0;
            RxBuffer[0] = 0x0;
        }
        break;
    case 2:
        New_CMD_length = Rx_Temp + 2;
        if (New_CMD_length >= PTO_MAX_BUF_LEN)
        {
            // printf("PTO_MAX_BUF_LEN ERR:%#2x\n", New_CMD_length);
            RxIndex = 0;
            RxFlag = 0;
            RxBuffer[0] = 0;
            RxBuffer[1] = 0;
            New_CMD_length = 0;
            break;
        }
        RxBuffer[RxIndex] = Rx_Temp;
        // printf("PTO_MAX_BUF_LEN:%#2x",RxBuffer[RxIndex]);
        RxIndex++;
        RxFlag = 3;
        break;

    case 3:
        RxBuffer[RxIndex] = Rx_Temp;
        // printf("data:%#2x",RxBuffer[RxIndex]);
        RxIndex++;
        if (RxIndex >= New_CMD_length)
        {
            // printf("124\n");
            New_CMD_flag = 1;
            RxIndex = 0;
            RxFlag = 0;
        }
        break;

    default:
        break;
    }
}

static uint8_t calculate_checksum(uint8_t* data,uint8_t len){
    uint8_t checksum=0;
    for(uint8_t i=2;i<len-1;i++){
        checksum+=data[i];
    }
    return checksum;
}


void Usart1_on_recv(uint8_t *recv_buffer, uint8_t recv_len)
{
    if (recv_buffer[0] == 0xFF && (recv_buffer[1] == 0xFC || recv_buffer[1] == 0xFB))
    {
        uint8_t packet_length = recv_buffer[2];
        // printf("in bag head\n");
        // Motor_Set_Pwm(0,50);
        if ( recv_len >= (packet_length + 2))
        {
            // 计算并验证校验和
            uint8_t function_code = recv_buffer[3];
            uint8_t received_checksum = recv_buffer[recv_len-1];
            uint8_t calculated_checksum = calculate_checksum(recv_buffer, recv_len);
            // printf("in recv_buffer\n");
            if (received_checksum == calculated_checksum)
            {
                switch (function_code)
                {
                    case 0x10: // 电机PWM控制功能，对应上层set_motor
                        {
                            // printf("in case 0x1\n");
                            set_motor(recv_buffer,recv_len);
                            break;
                        }
                    case 0x12:
                        {
                            // printf("in case 0x12\n");
                            // Motion_Set_Pwn(20,20,20,20);
                            set_car_motion(recv_buffer,recv_len);
                            break;
                        }
                    case 0x36:
                        {
                            set_tracing_state(recv_buffer,recv_len);
                            break;
                        }
                    case 0x02:
                        {
                            set_beeping(recv_buffer,recv_len);
                            break;
                        }
                    case 0x11:
                        {
                            set_car_run(recv_buffer,recv_len);
                            break;
                        }
                    default:
                    {
                        printf("no such function:%d",function_code);
                    }
                }
                
            }
        }
    }
}

/********************************************************************************************************/
// 解析数据
void Upper_Data_Parse(uint8_t *data_buf, uint8_t num)
{

    int sum = 0;
    for (uint8_t i = 2; i < (num - 1); i++)
        sum += *(data_buf + i);
    sum = sum & 0xFF;
    /* 判断校验累加和 若不同则舍弃*/
    uint8_t recvSum = *(data_buf + num - 1);
    if (!(sum == recvSum))
    {
        // printf("Check sum error!, CalSum:%d, recvSum:%d\n", sum, recvSum);
        // for (uint8_t i = 0; i < num; i++)
        // {
        //     printf("data_buf[%d]:%#2x    ", i, data_buf[i]);
        // }
        // printf("\n");
        return;
    }

    uint8_t func_id = *(data_buf + 3);

    switch (func_id)
    {
    /* 判断功能字：蜂鸣器控制 */
    case FUNC_BEEP:
    {
        uint16_t time = *(data_buf + 5) << 8 | *(data_buf + 4);
        // printf("beep:%d\n", time);
        bsp_beep_play(time);
        break;
    }
    /* 控制电机，未使用编码器 */
    case FUNC_MOTOR:
    {
        int16_t speed[4] = {0};
        int8_t motor_1 = *(data_buf + 4);
        int8_t motor_2 = *(data_buf + 5);
        int8_t motor_3 = *(data_buf + 6);
        int8_t motor_4 = *(data_buf + 7);

        // printf("motor=%d, %d, %d, %d", motor_1, motor_2, motor_3, motor_4);

        int16_t motor_pulse = MOTOR_MAX_PULSE - MOTOR_IGNORE_PULSE;
        speed[0] = (int16_t)motor_1 * (motor_pulse / 100.0);
        speed[1] = (int16_t)motor_2 * (motor_pulse / 100.0);
        speed[2] = (int16_t)motor_3 * (motor_pulse / 100.0);
        speed[3] = (int16_t)motor_4 * (motor_pulse / 100.0);
        // PWM控制小车运动
        Motion_Set_Pwm(speed[0], speed[1], speed[2], speed[3]);
        break;
    }
    /* 控制小车运动 */
    case FUNC_CAR_RUN:
    {
        uint8_t parm = *(data_buf + 4);
        uint8_t state = *(data_buf + 5);
        uint16_t speed = *(data_buf + 7) << 8 | *(data_buf + 6);
        // printf("car_run=0x%02X, %d, %d", parm, state, speed);
        uint8_t adjust = parm & 0x80;
        Motion_Ctrl_State(state, speed, (adjust == 0 ? 0 : 1));
        break;
    }

    /* 判断功能字：小车速度设置 */
    case FUNC_MOTION:
    {
        uint8_t parm = (uint8_t)*(data_buf + 4);
        int16_t Vx_recv = *(data_buf + 6) << 8 | *(data_buf + 5);
        int16_t Vy_recv = *(data_buf + 8) << 8 | *(data_buf + 7);
        int16_t Vz_recv = *(data_buf + 10) << 8 | *(data_buf + 9);
        uint8_t adjust = parm & 0x80;
        // printf("motion: 0x%02X, %d, %d, %d\n", parm, Vx_recv, Vy_recv, Vz_recv);

        if (Vx_recv == 0 && Vy_recv == 0 && Vz_recv == 0)
        {
            Motion_Stop(STOP_BRAKE);
        }
        else
        {
            Motion_Ctrl(Vx_recv, Vy_recv, Vz_recv, (adjust == 0 ? 0 : 1));
        }
        // set_car_motion(data_buf,num);
        break;
    }

    /* 判断功能字：PID参数设置 */
    case FUNC_SET_MOTOR_PID:
    {
        uint16_t kp_recv = *(data_buf + 5) << 8 | *(data_buf + 4);
        uint16_t ki_recv = *(data_buf + 7) << 8 | *(data_buf + 6);
        uint16_t kd_recv = *(data_buf + 9) << 8 | *(data_buf + 8);
        uint8_t args = *(data_buf + 10);
        float kp = kp_recv / 1000.0;
        float ki = ki_recv / 1000.0;
        float kd = kd_recv / 1000.0;
        // printf("pid:%.2f, %.2f, %.2f, args:0x%02X\n", kp, ki, kd, args);
        PID_Set_Motor_Parm(MAX_MOTOR, kp, ki, kd);
        // if (args == SAVE_VERIFY)
        // {
        //     Flash_Set_PID(MAX_MOTOR, kp, ki, kd);
        // }
        break;
    }
    /* 判断功能字：偏航角PID参数设置 */
    case FUNC_SET_YAW_PID:
    {
        uint16_t kp_recv = *(data_buf + 5) << 8 | *(data_buf + 4);
        uint16_t ki_recv = *(data_buf + 7) << 8 | *(data_buf + 6);
        uint16_t kd_recv = *(data_buf + 9) << 8 | *(data_buf + 8);
        uint8_t forever = *(data_buf + 10);
        float kp = kp_recv / 1000.0;
        float ki = ki_recv / 1000.0;
        float kd = kd_recv / 1000.0;
        // printf("YAW PID:%.2f, %.2f, %.2f, args:0x%02X\n", kp, ki, kd, forever);
        PID_Yaw_Set_Parm(kp, ki, kd);
        // if (forever == SAVE_VERIFY)
        // {
        //     Flash_Set_Yaw_PID(kp, ki, kd);
        // }
        break;
    }
    /* 判断功能字：寻线模式 */
    case FUN_CAR_TRACE:
    {
        uint8_t parm = (uint8_t)*(data_buf + 4);
        // printf("[trace:%#02x]\n", parm);
        // Trace_Ctrl(parm);
        break;
    }
    default:
        break;
    }
}
