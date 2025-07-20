#include "app_motion.h"

// 编码器10毫秒前后数据
int g_Encoder_All_Now[MAX_MOTOR] = {0};
int g_Encoder_All_Last[MAX_MOTOR] = {0};

int g_Encoder_All_Offset[MAX_MOTOR] = {0};

uint8_t g_start_ctrl = 0;

car_data_t car_data;
motor_data_t motor_data;

uint8_t g_yaw_adjust = 0;
#if YABO_CAR
car_type_t g_car_type = CAR_MECANUM; // 亚博
#endif

#if RUNHE_CAR
car_type_t g_car_type = CAR_MECANUM_MY; // 润和
#endif

static float Motion_Get_Circle_Pulse(void)
{
    float temp = 0;
    switch (g_car_type)
    {
    case CAR_MECANUM:
        temp = ENCODER_CIRCLE_330;
        break;
    case CAR_MECANUM_MY:
        temp = ENCODER_CIRCLE_111;
        break;

    default:
        temp = ENCODER_CIRCLE_111;
        break;
    }
    return temp;
}
// 小车停止
void Motion_Stop(uint8_t brake)
{
    Motion_Set_Speed(0, 0, 0, 0);
    PID_Clear_Motor(MAX_MOTOR);
    g_start_ctrl = 0;
    g_yaw_adjust = 0;
    Motor_Stop(brake);
}
// speed_mX=[-1000, 1000], 单位为：mm/s
void Motion_Set_Speed(int16_t speed_m1, int16_t speed_m2, int16_t speed_m3, int16_t speed_m4)
{
    // printf("Motion_Set_Speed \n");
    g_start_ctrl = 1;
    motor_data.speed_set[0] = speed_m1;
    motor_data.speed_set[1] = speed_m2;
    motor_data.speed_set[2] = speed_m3;
    motor_data.speed_set[3] = speed_m4;
    for (uint8_t i = 0; i < MAX_MOTOR; i++)
    {
        PID_Set_Motor_Target(i, motor_data.speed_set[i] * 1.0);
    }
}
void Motion_Ctrl(int16_t V_x, int16_t V_y, int16_t V_z, uint8_t adjust)
{
    switch (g_car_type)
    {
    case CAR_MECANUM:
    {
        Mecanum_Ctrl(V_x, V_y, V_z, adjust);
        break;
    }
    case CAR_MECANUM_MY:
    {
        if (V_x > CAR_X3_PLUS_MAX_SPEED)
            V_x = CAR_X3_PLUS_MAX_SPEED;
        if (V_x < -CAR_X3_PLUS_MAX_SPEED)
            V_x = -CAR_X3_PLUS_MAX_SPEED;
        if (V_y > CAR_X3_PLUS_MAX_SPEED)
            V_y = CAR_X3_PLUS_MAX_SPEED;
        if (V_y < -CAR_X3_PLUS_MAX_SPEED)
            V_y = -CAR_X3_PLUS_MAX_SPEED;
        Mecanum_Ctrl(V_x, V_y, V_z, adjust);
        break;
    }
    default:
        break;
    }
}
// 增加偏航角校准小车运动方向
void Motion_Yaw_Calc(float yaw)
{
    switch (g_car_type)
    {
    case CAR_MECANUM:
    {
        Mecanum_Yaw_Calc(yaw);
        break;
    }
    case CAR_MECANUM_MY:
    {
        Mecanum_Yaw_Calc(yaw);
        break;
    }

    default:
        break;
    }
}

float MPU_Get_Yaw_Now(void)
{
    return 0.0; // 如果没有MPU传感器，返回0
}

uint8_t Motion_Get_Car_Type(void)
{
    return (uint8_t)g_car_type;
}
// 从编码器读取当前各轮子速度，单位mm/s
void Motion_Get_Speed(car_data_t *car)
{
    int i = 0;
    float speed_mm[MAX_MOTOR] = {0};
    float circle_mm = Motion_Get_Circle_MM();
    float circle_pulse = Motion_Get_Circle_Pulse();
    float robot_APB = Motion_Get_APB();

    Motion_Get_Encoder();

    // 计算轮子速度，单位mm/s。
    for (i = 0; i < 4; i++)
    {
        speed_mm[i] = (g_Encoder_All_Offset[i]) * 100 * circle_mm / circle_pulse;
        // printf(" speed_mm[%d]:%.02f ,",i,speed_mm[i]);
    }
    switch (g_car_type)
    {
    case CAR_MECANUM:
    {
        car->Vx = (speed_mm[0] + speed_mm[1] + speed_mm[2] + speed_mm[3]) / 4.0;
        car->Vy = -(speed_mm[0] - speed_mm[1] - speed_mm[2] + speed_mm[3]) / 4;
        // printf("car->Vx:%d ,car->Vy:%d  \n",car->Vx ,car->Vy);
        car->Vz = -(speed_mm[0] + speed_mm[1] - speed_mm[2] - speed_mm[3]) / 4.0f / robot_APB * 1000;
        break;
    }
    case CAR_MECANUM_MY:
    {
        car->Vx = (speed_mm[0] + speed_mm[1] + speed_mm[2] + speed_mm[3]) / 4.0;
        car->Vy = -(speed_mm[0] - speed_mm[1] - speed_mm[2] + speed_mm[3]) / 4.0;
        car->Vz = -(speed_mm[0] + speed_mm[1] - speed_mm[2] - speed_mm[3]) / 4.0f / robot_APB * 1000;
        break;
    }

    default:
        break;
    }

    if (g_start_ctrl)
    {
        for (i = 0; i < MAX_MOTOR; i++)
        {
            motor_data.speed_mm_s[i] = speed_mm[i];
        }
        if (g_yaw_adjust)
        {
            Motion_Yaw_Calc(MPU_Get_Yaw_Now());
        }
        PID_Calc_Motor(&motor_data);
    }
}
void Get_speed_data(void)
{
    printf("car->Vx:%d ,car->Vy:%d,car->Vz:%d\n", car_data.Vx, car_data.Vy, car_data.Vz);
}
// 控制小车运动，Motor_X=[-4000, 4000]，超过范围则无效。
void Motion_Set_Pwm(int16_t Motor_1, int16_t Motor_2, int16_t Motor_3, int16_t Motor_4)
{
    // printf("Motion_Set_Pwm \n");
    if (Motor_1 >= -MOTOR_MAX_PULSE && Motor_1 <= MOTOR_MAX_PULSE)
    {
        Motor_Set_Pwm(MOTOR_ID_M1, Motor_1);
    }
    if (Motor_2 >= -MOTOR_MAX_PULSE && Motor_2 <= MOTOR_MAX_PULSE)
    {
        Motor_Set_Pwm(MOTOR_ID_M2, Motor_2);
    }
    if (Motor_3 >= -MOTOR_MAX_PULSE && Motor_3 <= MOTOR_MAX_PULSE)
    {
        Motor_Set_Pwm(MOTOR_ID_M3, Motor_3);
    }
    if (Motor_4 >= -MOTOR_MAX_PULSE && Motor_4 <= MOTOR_MAX_PULSE)
    {
        Motor_Set_Pwm(MOTOR_ID_M4, Motor_4);
    }
}
// 设置偏航角状态，如果使能则刷新target目标角度。
void Motion_Set_Yaw_Adjust(uint8_t adjust)
{
    if (adjust == 0)
    {
        g_yaw_adjust = 0;
    }
    else
    {
        g_yaw_adjust = 1;
    }
    // if (g_yaw_adjust)
    // {

    //     PID_Yaw_Reset(MPU_Get_Yaw_Now());
    // }
}

// 获取编码器数据，并计算偏差脉冲数
void Motion_Get_Encoder(void)
{
    Encoder_Get_ALL(g_Encoder_All_Now);
    // printf("1:%d ,2:%d , 3:%d ,4:%d :  \n",g_Encoder_All_Now[0],g_Encoder_All_Now[1],g_Encoder_All_Now[2],g_Encoder_All_Now[3]);

    for (uint8_t i = 0; i < MAX_MOTOR; i++)
    {
        // 记录两次测试时间差的脉冲数
        g_Encoder_All_Offset[i] = g_Encoder_All_Now[i] - g_Encoder_All_Last[i];
        // 记录上次编码器数据
        g_Encoder_All_Last[i] = g_Encoder_All_Now[i];
    }
}

// 返回当前小车轮子轴间距和的一半
float Motion_Get_APB(void)
{
    if (g_car_type == CAR_MECANUM)
        return MECANUM_APB;
    if (g_car_type == CAR_MECANUM_MY)
        return MECANUM_RH_APB;

    return MECANUM_RH_APB;
}
// 返回当前小车轮子转一圈的多少毫米
float Motion_Get_Circle_MM(void)
{
    if (g_car_type == CAR_MECANUM)
        return MECANUM_CIRCLE_MM;
    if (g_car_type == CAR_MECANUM_MY)
        return MECANUM_RH_CIRCLE_MM;

    return MECANUM_RH_CIRCLE_MM;
}
// 控制小车的运动状态
void Motion_Ctrl_State(uint8_t state, uint16_t speed, uint8_t adjust)
{
    uint16_t input_speed;
    switch (g_car_type)
    {
    case CAR_MECANUM:
    {
        input_speed = speed * 10;
        Mecanum_State(state, input_speed, adjust);
        break;
    }
    case CAR_MECANUM_MY:
    {

        // printf("Motion_Ctrl_State \n");
        input_speed = speed * 10;
        if (input_speed > CAR_X3_PLUS_MAX_SPEED)
            input_speed = CAR_X3_PLUS_MAX_SPEED;
        Mecanum_State(state, input_speed, adjust);
        break;
    }
    default:
        break;
    }
}

void Motion_Send_Data(void)
{
#define LEN 12
    uint8_t data_buffer[LEN] = {0};
    uint8_t i, checknum = 0;
#if UART1_SEND_DATA
    Serial1_SendByte(0xFF);
    Serial1_SendByte(0XFB);
    Serial1_SendByte(0x0A);
    Serial1_SendByte(0x0A);

    Serial1_SendByte(car_data.Vx & 0xff);
    Serial1_SendByte((car_data.Vx >> 8) & 0xff);
    Serial1_SendByte(car_data.Vy & 0xff);
    Serial1_SendByte((car_data.Vy >> 8) & 0xff);
    Serial1_SendByte(car_data.Vz & 0xff);
    Serial1_SendByte((car_data.Vz >> 8) & 0xff);
    Serial1_SendByte(0x71);
#endif
    data_buffer[0] = 0xFF;
    data_buffer[1] = 0XFB;
    data_buffer[2] = LEN - 2; // 数量
    data_buffer[3] = 0x0A;    // 功能位
    data_buffer[4] = car_data.Vx & 0xff;
    data_buffer[5] = (car_data.Vx >> 8) & 0xff;
    data_buffer[6] = car_data.Vy & 0xff;
    data_buffer[7] = (car_data.Vy >> 8) & 0xff;
    data_buffer[8] = car_data.Vz & 0xff;
    data_buffer[9] = (car_data.Vz >> 8) & 0xff;
    data_buffer[10] = 0x71; // 依赖于系统电压检测;

    for (i = 2; i < LEN - 1; i++)
    {
        checknum += data_buffer[i];
    }
    data_buffer[LEN - 1] = checknum;
#if UART1_SEND_DATA
    Serial1_SendByte(checknum);
#endif

#if DMA_SEND_DATA
    usartdmasend(data_buffer, sizeof(data_buffer));
    // Get_speed_data();
#endif
}
void Request_Data(void)
{
#define LEN_Data 7
    uint8_t data_buffer[LEN_Data] = {0};
    uint8_t i, checknum = 0;
    data_buffer[0] = 0xFF;
    data_buffer[1] = 0XFB;
    data_buffer[2] = 0x05; // 数量
    data_buffer[3] = 0x51; // 功能位
    data_buffer[4] = 0x03; // 功能位
    data_buffer[5] = 0x05; // 功能位
#if UART1_SEND_DATA
    Serial1_SendByte(0xFF);
    Serial1_SendByte(0XFB);
    Serial1_SendByte(0x05);
    Serial1_SendByte(0x51);
    Serial1_SendByte(0x03);
    Serial1_SendByte(0x05);
#endif

    for (i = 2; i < LEN_Data - 1; i++)
    {
        checknum += data_buffer[i];
    }
    data_buffer[LEN_Data - 1] = checknum;
#if DMA_SEND_DATA
    usartdmasend(data_buffer, sizeof(data_buffer));
#endif
#if UART1_SEND_DATA
    Serial1_SendByte(checknum);
#endif
}

// 运动控制句柄，每10ms调用一次，主要处理速度相关的数据
void Motion_Handle(void)
{
    Motion_Get_Speed(&car_data);
    // printf("g_start_ctrl : %d ",g_start_ctrl);
    if (g_start_ctrl)
    {
        Motion_Set_Pwm(motor_data.speed_pwm[0], motor_data.speed_pwm[1], motor_data.speed_pwm[2], motor_data.speed_pwm[3]);
    }
}