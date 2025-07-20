#include "app_angle.h"
#include "app_math.h"

_Matrix Mat = {0};

imu_data_t g_imu_data;
static const float Gyro_Gr = 1 / 16.384f; // 角速度变成弧度	此参数对应陀螺500度每秒0.00026646f
static const float Acc_Gr = 1 / 208.980f / 9.8;
float Atmpe_Y, Atmpe_X;
#define MahonyPERIOD 10.0f // 姿态解算周期（ms）
#define kp 0.5f            // 比例增益决定收敛速率与加速度计/磁计计的速率
#define ki 0.0001f         // 整体增益控制陀螺仪偏置收敛速率

float q0 = 1, q1 = 0, q2 = 0, q3 = 0;  // 代表估计方向的四元素元素
float exInt = 0, eyInt = 0, ezInt = 0; // 缩放的积分误差

/*
 * 函数名：get_iir_factor
 * 描述  ：求取IIR滤波器的滤波因子
 * 输入  ：out_factor滤波因子首地址，Time任务执行周期，Cut_Off滤波截止频率
 * 返回  ：
 */
void get_iir_factor(float *out_factor, float Time, float Cut_Off)
{
    *out_factor = Time / (Time + 1 / (2.0f * M_PI * Cut_Off));
}
/**
 * @brief   IIR低通滤波器
 * @param   *acc_in 输入三轴数据指针变量
 * @param   *acc_out 输出三轴数据指针变量
 * @param   lpf_factor 滤波因数
 * @retval  x
 */
float iir_lpf(float in, float out, float lpf_factor)
{
    // printf("iir_lpf_in   %f\n",in);
    out = out + lpf_factor * (in - out);
    return out;
}
/*
 * 函数名：mahony_update
 * 描述  ：姿态解算
 * 输入  ：陀螺仪三轴数据（单位：弧度/秒），加速度三轴数据（单位：g）
 * 返回  ：
 */
// Gyroscope units are radians/second, accelerometer  units are irrelevant as the vector is normalised.
void mahony_update(float gx, float gy, float gz, float ax, float ay, float az)
{
    float norm;
    float vx, vy, vz;
    float ex, ey, ez;

    if (ax * ay * az == 0)
        return;
    gx = gx * (M_PI / 180.0f);
    gy = gy * (M_PI / 180.0f);
    gz = gz * (M_PI / 180.0f);
    //[ax,ay,az]是机体坐标系下加速度计测得的重力向量(竖直向下)
    norm = invSqrt(ax * ax + ay * ay + az * az);
    ax = ax * norm;
    ay = ay * norm;
    az = az * norm;

    // VectorA = MatrixC * VectorB
    // VectorA ：参考重力向量转到在机体下的值
    // MatrixC ：地理坐标系转机体坐标系的旋转矩阵
    // VectorB ：参考重力向量（0,0,1）
    //[vx,vy,vz]是地理坐标系重力分向量[0,0,1]经过DCM旋转矩阵(C(n->b))计算得到的机体坐标系中的重力向量(竖直向下)

    vx = Mat.DCM_T[0][2];
    vy = Mat.DCM_T[1][2];
    vz = Mat.DCM_T[2][2];

    // 机体坐标系下向量叉乘得到误差向量，误差e就是测量得到的vˉ和预测得到的 v^之间的相对旋转。这里的vˉ就是[ax,ay,az]’,v^就是[vx,vy,vz]’
    // 利用这个误差来修正DCM方向余弦矩阵(修正DCM矩阵中的四元素)，这个矩阵的作用就是将b系和n正确的转化直到重合。
    // 实际上这种修正方法只把b系和n系的XOY平面重合起来，对于z轴旋转的偏航，加速度计无可奈何，
    // 但是，由于加速度计无法感知z轴上的旋转运动，所以还需要用地磁计来进一步补偿。
    // 两个向量的叉积得到的结果是两个向量的模与他们之间夹角正弦的乘积a×v=|a||v|sinθ,
    // 加速度计测量得到的重力向量和预测得到的机体重力向量已经经过单位化，因而他们的模是1，
    // 也就是说它们向量的叉积结果仅与sinθ有关，当角度很小时，叉积结果可以近似于角度成正比。

    ex = ay * vz - az * vy;
    ey = az * vx - ax * vz;
    ez = ax * vy - ay * vx;

    // 对误差向量进行积分
    exInt = exInt + ex * ki;
    eyInt = eyInt + ey * ki;
    ezInt = ezInt + ez * ki;

    // 姿态误差补偿到角速度上，修正角速度积分漂移，通过调节Kp、Ki两个参数，可以控制加速度计修正陀螺仪积分姿态的速度。
    gx = gx + kp * ex + exInt;
    gy = gy + kp * ey + eyInt;
    gz = gz + kp * ez + ezInt;

    // 一阶龙格库塔法更新四元数
    q0 = q0 + (-q1 * gx - q2 * gy - q3 * gz) * MahonyPERIOD * 0.0005f;
    q1 = q1 + (q0 * gx + q2 * gz - q3 * gy) * MahonyPERIOD * 0.0005f;
    q2 = q2 + (q0 * gy - q1 * gz + q3 * gx) * MahonyPERIOD * 0.0005f;
    q3 = q3 + (q0 * gz + q1 * gy - q2 * gx) * MahonyPERIOD * 0.0005f;

    // 把上述运算后的四元数进行归一化处理。得到了物体经过旋转后的新的四元数。
    norm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 = q0 * norm;
    q1 = q1 * norm;
    q2 = q2 * norm;
    q3 = q3 * norm;

    // 四元素转欧拉角
    g_imu_data.pitch = atan2(2.0f * (q0 * q1 + q2 * q3), q0 * q0 - q1 * q1 - q2 * q2 + q3 * q3) * (180.0f / M_PI);
    g_imu_data.roll = -asin(2.0f * (q0 * q2 - q1 * q3)) * (180.0f / M_PI);

    // z轴角速度积分的偏航角
    g_imu_data.yaw += g_imu_data.deg_s.z * MahonyPERIOD * 0.001f;
}
/*
 * 函数名：rotation_matrix
 * 描述  ：旋转矩阵：机体坐标系 -> 地理坐标系
 * 输入  ：
 * 返回  ：
 */
void rotation_matrix(void)
{
    Mat.DCM[0][0] = 1.0f - 2.0f * q2 * q2 - 2.0f * q3 * q3;
    Mat.DCM[0][1] = 2.0f * (q1 * q2 - q0 * q3);
    Mat.DCM[0][2] = 2.0f * (q1 * q3 + q0 * q2);

    Mat.DCM[1][0] = 2.0f * (q1 * q2 + q0 * q3);
    Mat.DCM[1][1] = 1.0f - 2.0f * q1 * q1 - 2.0f * q3 * q3;
    Mat.DCM[1][2] = 2.0f * (q2 * q3 - q0 * q1);

    Mat.DCM[2][0] = 2.0f * (q1 * q3 - q0 * q2);
    Mat.DCM[2][1] = 2.0f * (q2 * q3 + q0 * q1);
    Mat.DCM[2][2] = 1.0f - 2.0f * q1 * q1 - 2.0f * q2 * q2;
}
/*
 * 函数名：rotation_matrix_T
 * 描述  ：旋转矩阵的转置矩阵：地理坐标系 -> 机体坐标系
 * 输入  ：
 * 返回  ：
 */
void rotation_matrix_T(void)
{
    Mat.DCM_T[0][0] = 1.0f - 2.0f * q2 * q2 - 2.0f * q3 * q3;
    Mat.DCM_T[0][1] = 2.0f * (q1 * q2 + q0 * q3);
    Mat.DCM_T[0][2] = 2.0f * (q1 * q3 - q0 * q2);

    Mat.DCM_T[1][0] = 2.0f * (q1 * q2 - q0 * q3);
    Mat.DCM_T[1][1] = 1.0f - 2.0f * q1 * q1 - 2.0f * q3 * q3;
    Mat.DCM_T[1][2] = 2.0f * (q2 * q3 + q0 * q1);

    Mat.DCM_T[2][0] = 2.0f * (q1 * q3 + q0 * q2);
    Mat.DCM_T[2][1] = 2.0f * (q2 * q3 - q0 * q1);
    Mat.DCM_T[2][2] = 1.0f - 2.0f * q1 * q1 - 2.0f * q2 * q2;
}
/*
 * 函数名：Matrix_ready
 * 描述  ：矩阵更新准备，为姿态解算使用
 * 输入  ：
 * 返回  ：
 */
void Matrix_ready(void)
{
    rotation_matrix();   // 旋转矩阵更新
    rotation_matrix_T(); // 旋转矩阵的逆矩阵更新
}
void IIR_imu(void)
{
    get_iir_factor(&g_imu_data.att_acc_factor, 0.001, 25);
    get_iir_factor(&g_imu_data.att_gryo_factor, 0.001, 30);
}
short accel[3], gyro[3];
float _accel[3], _gyro[3];
float acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z;
/* 四元素获取  dt：10MS */
void get_attitude_angle()
{
    _accel[0] = g_imu_data.accX;
    _accel[1] = g_imu_data.accY;
    _accel[2] = g_imu_data.accZ;
    _gyro[0] = g_imu_data.gyroX;
    _gyro[1] = g_imu_data.gyroY;
    _gyro[2] = g_imu_data.gyroZ;
    // 添加死区
    _gyro[0] = (fabs(_gyro[0]) < 4) ? (0) : (_gyro[0]);
    _gyro[1] = (fabs(_gyro[1]) < 4) ? (0) : (_gyro[1]);
    _gyro[2] = (fabs(_gyro[2]) < 4) ? (0) : (_gyro[2]);

    acc_x = iir_lpf((float)(_accel[0]), (float)acc_x, g_imu_data.att_acc_factor);
    acc_y = iir_lpf((float)(_accel[1]), (float)acc_y, g_imu_data.att_acc_factor);
    acc_z = iir_lpf((float)(_accel[2]), (float)acc_z, g_imu_data.att_acc_factor);
    gyro_x = iir_lpf((float)(_gyro[0]), (float)gyro_x, g_imu_data.att_gryo_factor);
    gyro_y = iir_lpf((float)(_gyro[1]), (float)gyro_y, g_imu_data.att_gryo_factor);
    gyro_z = iir_lpf((float)(_gyro[2]), (float)gyro_z, g_imu_data.att_gryo_factor);
    // printf("%.3f,  %.3f,  %.3f,  %.3f,   %.3f,  %.3f\r\n", acc_x, acc_y, acc_z, gyro_x, gyro_y, gyro_z);
    // printf("_accel%f    _gyro%f\n", _accel[0], _gyro[0]);

    /*数据存储*/
    g_imu_data.acc_g.x = acc_x * Acc_Gr;
    g_imu_data.acc_g.y = acc_y * Acc_Gr;
    g_imu_data.acc_g.z = acc_z * Acc_Gr;
    g_imu_data.deg_s.x = gyro_x * Gyro_Gr * 2.0; //   gyro_x * Gyro_Gr/230*360
    g_imu_data.deg_s.y = gyro_y * Gyro_Gr * 2.0;
    g_imu_data.deg_s.z = gyro_z * Gyro_Gr * 2.0;
    // printf("%.3f,  %.3f,  %.3f,  %.3f,   %.3f,  %.3f\r\n", g_imu_data.acc_g.x, g_imu_data.acc_g.y, g_imu_data.acc_g.z, g_imu_data.deg_s.x, g_imu_data.deg_s.y, g_imu_data.deg_s.z);
    mahony_update(g_imu_data.deg_s.x, g_imu_data.deg_s.y, g_imu_data.deg_s.z,
                  g_imu_data.acc_g.x, g_imu_data.acc_g.y, g_imu_data.acc_g.z);
    Matrix_ready();
}
/*******************************************timer soft***********************************************/
// void timer1_Callback(void)
// {
//     MPU9250_Read_Data_Handle();
// 	// IMU();
// //   printf("enter timer1_Callback.......\n");
// }
// // osTimerId_t Timer_ID; // 定时器ID
// void soft_timer1_init()
// {
//   Timer_ID = osTimerNew(timer1_Callback, osTimerPeriodic, NULL, NULL); // 创建定时器
//   if (Timer_ID != NULL)
//   {
//     printf("ID = %d, Create Timer_ID is OK!\n", Timer_ID);

//     osStatus_t timerStatus = osTimerStart(Timer_ID, 1U); // 开始定时器， 并赋予定时器的定时值
//     if (timerStatus != osOK)
//     {
//       printf("timer is not startRun !\n");
//     }
//   }
// }