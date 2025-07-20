#ifndef __APP_ANGLE_H__
#define __APP_ANGLE_H__

/*******************************************include***********************************************/
#include "at32f403a_407.h"
#include "bsp_mpu9250.h"

/*******************************************define***********************************************/
typedef struct
{
    float x;
    float y;
    float z;
} SI_F_XYZ;

typedef struct
{
    int16_t accX;
    int16_t accY;
    int16_t accZ;
    int16_t gyroX;
    int16_t gyroY;
    int16_t gyroZ;
    int16_t magX;
    int16_t magY;
    int16_t magZ;

    SI_F_XYZ deg_s; // 度每秒
    SI_F_XYZ rad_s; // 弧度每秒
    SI_F_XYZ acc_g; // 加速度

    float att_acc_factor;
    float att_gryo_factor;

    float pitch;
    float roll;
    float yaw;

    int16_t Offset[6];
} imu_data_t;

typedef struct
{
    float DCM[3][3];   // 机体坐标系 -> 地理坐标系
    float DCM_T[3][3]; // 地理坐标系 -> 机体坐标系
} _Matrix;

extern imu_data_t g_imu_data;

/*******************************************function***********************************************/
// 函数声明
void get_attitude_angle();
void soft_timer1_init();


#endif /* __APP_ANGLE_H__ */
