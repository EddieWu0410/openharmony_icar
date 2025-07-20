#include "bsp_mpu9250.h"

#define ENABLE_MPU9250_FILTER 1
#define CONTROL_DELAY 100
// extern imu_data_t g_imu_data;
uint8_t g_mpu_init = 0;
uint8_t g_mpu_start = 0;
uint8_t g_mpu_test = 0;

// 零点漂移计数
int Deviation_Count = 0;
// 陀螺仪静差，原始数据
short Deviation_gyro[3], Original_gyro[3], Deviation_accel[3], Original_accel[3];

float pitch, roll, yaw; // 欧拉角
mpu_data_t mpu_data = {0};

// 控制MPU9250的IIC地址为0x68
void MPU_ADDR_CTRL(void)
{
	gpio_init_type gpio_init_struct;
	crm_periph_clock_enable(CRM_GPIOE_PERIPH_CLOCK, TRUE); // 开启GPIOE的时钟
	gpio_default_para_init(&gpio_init_struct);
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER; // 较大电流推动/吸入能力
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;				 // 推挽输出
	gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;						 // 复用
	gpio_init_struct.gpio_pins = GPIO_PINS_13;							 // PE13
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE;						 // 无上下拉
	gpio_init(GPIOE, &gpio_init_struct);
	gpio_bits_reset(GPIOE, GPIO_PINS_13);
}

static uint8_t MPU_Basic_Init(void)
{
	uint8_t res = 0;
	MPU_ADDR_CTRL();
	bsp_iic_soft_config();
	osDelay(10);
	MPU_Write_Byte(MPU9250_ADDR, MPU_PWR_MGMT1_REG, 0X80); // Reset MPU9250 //复位MPU9250
	osDelay(100);										   // Delay 100 ms //延时100ms
	MPU_Write_Byte(MPU9250_ADDR, MPU_PWR_MGMT1_REG, 0X00); // Wake mpu9250 //唤醒MPU9250
	MPU_Set_Gyro_Fsr(1);								   // Gyroscope sensor              //陀螺仪传感器,±500dps=±500°/s ±32768 (gyro/32768*500)*PI/180(rad/s)=gyro/3754.9(rad/s)
	MPU_Set_Accel_Fsr(0);								   // Acceleration sensor           //加速度传感器,±2g=±2*9.8m/s^2 ±32768 accel/32768*19.6=accel/1671.84
	MPU_Set_Rate(50);									   // Set the sampling rate to 50Hz //设置采样率50Hz
	MPU_Write_Byte(MPU9250_ADDR, MPU_INT_EN_REG, 0X00);	   // Turn off all interrupts //关闭所有中断
	MPU_Write_Byte(MPU9250_ADDR, MPU_USER_CTRL_REG, 0X00); // The I2C main mode is off //I2C主模式关闭
	MPU_Write_Byte(MPU9250_ADDR, MPU_FIFO_EN_REG, 0X00);   // Close the FIFO //关闭FIFO
	// The INT pin is low, enabling bypass mode to read the magnetometer directly
	// INT引脚低电平有效，开启bypass模式，可以直接读取磁力计
	MPU_Write_Byte(MPU9250_ADDR, MPU_INTBP_CFG_REG, 0X82);
	// Read the ID of MPU9250 读取MPU9250的ID
	res = MPU_Read_Byte(MPU9250_ADDR, MPU_DEVICE_ID_REG);
	printf("MPU9250 RD ID=0x%02X\n", res);
	if (res == MPU9250_ID) // The device ID is correct //器件ID正确
	{
		MPU_Write_Byte(MPU9250_ADDR, MPU_PWR_MGMT1_REG, 0X01); // Set CLKSEL,PLL X axis as reference //设置CLKSEL,PLL X轴为参考
		MPU_Write_Byte(MPU9250_ADDR, MPU_PWR_MGMT2_REG, 0X00); // Acceleration and gyroscope both work //加速度与陀螺仪都工作
		MPU_Set_Rate(50);									   // Set the sampling rate to 50Hz //设置采样率为50Hz
	}
	else
		return 1;

	res = MPU_Read_Byte(AK8963_ADDR, MAG_WIA); // Read AK8963ID //读取AK8963ID
	printf("AK8963 RD ID=0x%02X\n", res);
	if (res == AK8963_ID)
	{
		MPU_Write_Byte(AK8963_ADDR, MAG_CNTL1, 0X11); // Set AK8963 to single measurement mode //设置AK8963为单次测量模式
	}
	else
		return 2;
	osDelay(30);
	return 0;
}

// 初始化MPU9250
uint8_t MPU9250_Init(void)
{
	uint8_t res = 0;
	uint8_t count = 0;
	while (1)
	{
		res = MPU_Basic_Init();
		if (res == 0)
		{
			printf("MPU9250 Basic Init OK\n");
			while (Deviation_Count < CONTROL_DELAY)
			{
				MPU_Get_Gyroscope(mpu_data.gyro);
				MPU_Get_Accelerometer(mpu_data.accel);
				osDelay(10);
			}
			bsp_beep_swich(3,500);
			break;
		}
		else
		{
			count++;
			printf("MPU9250 Basic Init Error:%d\n", res);
			if (count > 5)
			{
				return 1;
			}
		}
		osDelay(200);
	}

	return 0;
}
void MPU_Init_Ok(void)
{
	g_mpu_init = 1;
}
// 设置MPU9250陀螺仪传感器满量程范围
// fsr:0,±250dps;1,±500dps;2,±1000dps;3,±2000dps
// 返回值:0,设置成功, 其他,设置失败
uint8_t MPU_Set_Gyro_Fsr(uint8_t fsr)
{
	return MPU_Write_Byte(MPU9250_ADDR, MPU_GYRO_CFG_REG, fsr << 3); // 设置陀螺仪满量程范围
}
// 设置MPU9250加速度传感器满量程范围
// fsr:0,±2g;1,±4g;2,±8g;3,±16g
// 返回值:0,设置成功, 其他,设置失败
uint8_t MPU_Set_Accel_Fsr(uint8_t fsr)
{
	return MPU_Write_Byte(MPU9250_ADDR, MPU_ACCEL_CFG_REG, fsr << 3); // 设置加速度传感器满量程范围
}
// 设置MPU9250的数字低通滤波器
//  lpf:数字低通滤波频率(Hz)
//  返回值:0,设置成功, 其他,设置失败
uint8_t MPU_Set_LPF(uint16_t lpf)
{
	uint8_t data = 0;
	if (lpf >= 188)
		data = 1;
	else if (lpf >= 98)
		data = 2;
	else if (lpf >= 42)
		data = 3;
	else if (lpf >= 20)
		data = 4;
	else if (lpf >= 10)
		data = 5;
	else
		data = 6;
	return MPU_Write_Byte(MPU9250_ADDR, MPU_CFG_REG, data); // 设置数字低通滤波器
}

// 设置MPU9250的采样率(假定Fs=1KHz)
//  rate:4~1000(Hz)
//  返回值:0,设置成功, 其他,设置失败
uint8_t MPU_Set_Rate(uint16_t rate)
{
	uint8_t data;
	if (rate > 1000)
		rate = 1000;
	if (rate < 4)
		rate = 4;
	data = 1000 / rate - 1;
	data = MPU_Write_Byte(MPU9250_ADDR, MPU_SAMPLE_RATE_REG, data); // 设置数字低通滤波器
	return MPU_Set_LPF(rate / 2);									// 自动设置LPF为采样率的一半
}

// 得到陀螺仪值(原始值)
//  gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
//  返回值:0,设置成功, 其他,设置失败
uint8_t MPU_Get_Gyroscope(short *gyro)
{
#if ENABLE_MPU9250_FILTER
	uint8_t buf[6], res;
	buf[0] = MPU_Read_Byte(MPU9250_ADDR, MPU_GYRO_XOUTH_REG);
	buf[1] = MPU_Read_Byte(MPU9250_ADDR, MPU_GYRO_XOUTL_REG);
	buf[2] = MPU_Read_Byte(MPU9250_ADDR, MPU_GYRO_YOUTH_REG);
	buf[3] = MPU_Read_Byte(MPU9250_ADDR, MPU_GYRO_YOUTL_REG);
	buf[4] = MPU_Read_Byte(MPU9250_ADDR, MPU_GYRO_ZOUTH_REG);
	buf[5] = MPU_Read_Byte(MPU9250_ADDR, MPU_GYRO_ZOUTL_REG);
	if (Deviation_Count < CONTROL_DELAY)
	{
		Deviation_Count++;
		// Read the gyroscope zero //读取陀螺仪零点
		Deviation_gyro[0] = (((uint16_t)buf[0] << 8) | buf[1]);
		Deviation_gyro[1] = (((uint16_t)buf[2] << 8) | buf[3]);
		Deviation_gyro[2] = (((uint16_t)buf[4] << 8) | buf[5]);
	}
	else
	{
		// if (g_mpu_start == 0)
		// {
		// 	g_mpu_start = 1;
		// 	Beep_On_Time(100);
		// 	DEBUG("OFFSET GYRO:%d, %d, %d", Deviation_gyro[0], Deviation_gyro[1], Deviation_gyro[2]);
		// }
		// Save the raw data
		// 保存原始数据
		Original_gyro[0] = (((uint16_t)buf[0] << 8) | buf[1]);
		Original_gyro[1] = (((uint16_t)buf[2] << 8) | buf[3]);
		Original_gyro[2] = (((uint16_t)buf[4] << 8) | buf[5]);
		// printf("zero drift data:|X%f		|Y%f		|Z%f		\n", Deviation_gyro[0], Deviation_gyro[1], Deviation_gyro[2]);
		// Removes zero drift data
		// 去除零点漂移的数据
		gyro[0] = Original_gyro[0] - (Deviation_gyro[0]);
		gyro[1] = Original_gyro[1] - (Deviation_gyro[1]);
		gyro[2] = Original_gyro[2] - (Deviation_gyro[2]);
	}
	return 0;
#else
	uint8_t buf[6], res;
	res = MPU_Read_Len(MPU9250_ADDR, MPU_GYRO_XOUTH_REG, 6, buf);
	if (res == 0)
	{
		gyro[0] = ((uint16_t)buf[0] << 8) | buf[1];
		gyro[1] = ((uint16_t)buf[2] << 8) | buf[3];
		gyro[2] = ((uint16_t)buf[4] << 8) | buf[5];
	}
	g_mpu_start = 1;
	return res;
#endif
}

// 得到加速度值(原始值)
//  gx,gy,gz:陀螺仪x,y,z轴的原始读数(带符号)
//  返回值:0,设置成功, 其他,设置失败
uint8_t MPU_Get_Accelerometer(short *accel)
{
#if ENABLE_MPU9250_FILTER
	uint8_t buf[6], res;
	buf[0] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_XOUTH_REG);
	buf[1] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_XOUTL_REG);
	// accel[0] = ((uint16_t)buf[0] << 8) | buf[1];
	buf[2] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_YOUTH_REG);
	buf[3] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_YOUTL_REG);
	// accel[1] = ((uint16_t)buf[2] << 8) | buf[3];
	buf[4] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_ZOUTH_REG);
	buf[5] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_ZOUTL_REG);
	// accel[2] = ((uint16_t)buf[4] << 8) | buf[5];
	if (Deviation_Count < CONTROL_DELAY)
	{
		Deviation_Count++;
		// Read the gyroscope zero //读取陀螺仪零点
		Deviation_accel[0] = (((uint16_t)buf[0] << 8) | buf[1]);
		Deviation_accel[1] = (((uint16_t)buf[2] << 8) | buf[3]);
		Deviation_accel[2] = (((uint16_t)buf[4] << 8) | buf[5]);
	}
	else
	{
		// if (g_mpu_start == 0)
		// {
		// 	g_mpu_start = 1;
		// 	Beep_On_Time(100);
		// 	DEBUG("OFFSET GYRO:%d, %d, %d", Deviation_gyro[0], Deviation_gyro[1], Deviation_gyro[2]);
		// }
		// Save the raw data
		// 保存原始数据
		Original_accel[0] = (((uint16_t)buf[0] << 8) | buf[1]);
		Original_accel[1] = (((uint16_t)buf[2] << 8) | buf[3]);
		Original_accel[2] = (((uint16_t)buf[4] << 8) | buf[5]);
		// printf("zero drift data:|X%f		|Y%f		|Z%f		\n", Deviation_gyro[0], Deviation_gyro[1], Deviation_gyro[2]);
		// Removes zero drift data
		// 去除零点漂移的数据
		accel[0] = Original_accel[0] - (Deviation_accel[0]);
		accel[1] = Original_accel[1] - (Deviation_accel[1]);
		accel[2] = Original_accel[2] - (Deviation_accel[2]) + 16384;//+g
	}
	return 0;
#else
	uint8_t buf[6], res;
	res = MPU_Read_Len(MPU9250_ADDR, MPU_GYRO_XOUTH_REG, 6, buf);
	if (res == 0)
	{
		gyro[0] = ((uint16_t)buf[0] << 8) | buf[1];
		gyro[1] = ((uint16_t)buf[2] << 8) | buf[3];
		gyro[2] = ((uint16_t)buf[4] << 8) | buf[5];
	}
	g_mpu_start = 1;
	return res;
#endif
	// uint8_t buf[6], res;
	// buf[0] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_XOUTH_REG);
	// buf[1] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_XOUTL_REG);
	// accel[0] = ((uint16_t)buf[0] << 8) | buf[1];
	// buf[2] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_YOUTH_REG);
	// buf[3] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_YOUTL_REG);
	// accel[1] = ((uint16_t)buf[2] << 8) | buf[3];
	// buf[4] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_ZOUTH_REG);
	// buf[5] = MPU_Read_Byte(MPU9250_ADDR, MPU_ACCEL_ZOUTL_REG);
	// accel[2] = ((uint16_t)buf[4] << 8) | buf[5];
	// return res;
}

// 得到磁力计值(原始值)
//  mx,my,mz:磁力计x,y,z轴的原始读数(带符号)
//  返回值:0,设置成功, 其他,设置失败
uint8_t MPU_Get_Magnetometer(short *mag)
{
	uint8_t buf[6], res;
	buf[0] = MPU_Read_Byte(AK8963_ADDR, MAG_XOUT_L);
	buf[1] = MPU_Read_Byte(AK8963_ADDR, MAG_XOUT_H);
	mag[0] = ((uint16_t)buf[1] << 8) | buf[0];
	buf[2] = MPU_Read_Byte(AK8963_ADDR, MAG_YOUT_L);
	buf[3] = MPU_Read_Byte(AK8963_ADDR, MAG_YOUT_H);
	mag[1] = ((uint16_t)buf[3] << 8) | buf[2];
	buf[4] = MPU_Read_Byte(AK8963_ADDR, MAG_ZOUT_L);
	buf[5] = MPU_Read_Byte(AK8963_ADDR, MAG_ZOUT_H);
	mag[2] = ((uint16_t)buf[5] << 8) | buf[4];
	MPU_Write_Byte(AK8963_ADDR, MAG_CNTL1, 0X11); // AK8963每次读完以后都需要重新设置为单次测量模式
	return res;
}
// 获取当前翻滚角
float MPU_Get_Roll_Now(void)
{
	return mpu_data.roll * 0.01745; // 返回弧度
									// return mpu_data.roll*RtA;  // 返回角度
}

// 获取当前俯仰角
float MPU_Get_Pitch_Now(void)
{
	return mpu_data.pitch * 0.01745; // 返回弧度* 0.01745
									 // return mpu_data.pitch*RtA;  // 返回角度
}
// 获取当前偏航角
float MPU_Get_Yaw_Now(void)
{
	return mpu_data.yaw * 0.01745; // 返回弧度
								   // return mpu_data.yaw*RtA;    // 返回角度
}
// MPU9250读取数据线程
void MPU9250_Read_Data_Handle(void)
{
	if (Deviation_Count >= CONTROL_DELAY)
	{
		MPU_Get_Gyroscope(mpu_data.gyro);
		MPU_Get_Accelerometer(mpu_data.accel);
		MPU_Get_Magnetometer(mpu_data.compass);

		g_imu_data.accX = mpu_data.accel[0];
		g_imu_data.accY = mpu_data.accel[1];
		g_imu_data.accZ = mpu_data.accel[2];
		g_imu_data.gyroX = mpu_data.gyro[0];
		g_imu_data.gyroY = mpu_data.gyro[1];
		g_imu_data.gyroZ = mpu_data.gyro[2];

		// 姿态解算得到姿态角
		get_attitude_angle();
#if ENABLE_ROLL_PITCH
		mpu_data.roll = g_imu_data.roll;
		mpu_data.pitch = g_imu_data.pitch;
#endif
		mpu_data.yaw = g_imu_data.yaw;
#if ENABLE_DEBUG_MPU_ATT
		// printf("roll:%f, pitch:%f, yaw:%f\n", mpu_data.roll * 57.3, mpu_data.pitch * 57.3, mpu_data.yaw * 57.3);
		// printf("roll pitch yaw:%f,%f,%f\n", mpu_data.roll * 57.3, mpu_data.pitch * 57.3, mpu_data.yaw * 57.3);
		printf("roll pitch yaw:%f,%f,%f\n", mpu_data.roll, mpu_data.pitch, mpu_data.yaw);
#endif
	}
	else
	{
		MPU_Get_Gyroscope(mpu_data.gyro);
		MPU_Get_Accelerometer(mpu_data.accel);
		MPU_Get_Magnetometer(mpu_data.compass);
	}
}

void Get_mpu9250_value(void)
{

#define MPU_LEN 23

	uint8_t data_buffer[MPU_LEN] = {0};
	uint8_t i, checknum = 0;
	data_buffer[0] = 0xFF;
	data_buffer[1] = 0xFB;
	data_buffer[2] = MPU_LEN - 2;
	data_buffer[3] = 0x0B;

	data_buffer[4] = mpu_data.gyro[0] & 0xff;
	data_buffer[5] = (mpu_data.gyro[0] >> 8) & 0xff;
	data_buffer[6] = mpu_data.gyro[1] & 0xff;
	data_buffer[7] = (mpu_data.gyro[1] >> 8) & 0xff;
	data_buffer[8] = mpu_data.gyro[2] & 0xff;
	data_buffer[9] = (mpu_data.gyro[2] >> 8) & 0xff;

	data_buffer[10] = mpu_data.accel[0] & 0xff;
	data_buffer[11] = (mpu_data.accel[0] >> 8) & 0xff;
	data_buffer[12] = mpu_data.accel[1] & 0xff;
	data_buffer[13] = (mpu_data.accel[1] >> 8) & 0xff;
	data_buffer[14] = mpu_data.accel[2] & 0xff;
	data_buffer[15] = (mpu_data.accel[2] >> 8) & 0xff;

	data_buffer[16] = mpu_data.compass[0] & 0xff;
	data_buffer[17] = (mpu_data.compass[0] >> 8) & 0xff;
	data_buffer[18] = mpu_data.compass[1] & 0xff;
	data_buffer[19] = (mpu_data.compass[1] >> 8) & 0xff;
	data_buffer[20] = mpu_data.compass[2] & 0xff;
	data_buffer[21] = (mpu_data.compass[2] >> 8) & 0xff;
	for (i = 2; i < MPU_LEN - 1; i++)
	{
		checknum += data_buffer[i];
	}
	data_buffer[MPU_LEN - 1] = checknum; // 依赖于系统电压检测;

	usartdmasend(data_buffer, sizeof(data_buffer));
}
// 发送姿态角数据到主控，单位：弧度
void Get_Roll_Pitch(void)
{
#define Roll_Pitch_LEN 11

	uint8_t data_buffer[Roll_Pitch_LEN] = {0};
	uint8_t i, checknum = 0;
	data_buffer[0] = 0xFF;
	data_buffer[1] = 0XFB;
	data_buffer[2] = Roll_Pitch_LEN - 2; // 数量
	data_buffer[3] = 0x0C;				 // 功能位
	data_buffer[4] = (int)(MPU_Get_Roll_Now() * 10000) & 0xff;
	data_buffer[5] = ((int)(MPU_Get_Roll_Now() * 10000) >> 8) & 0xff;
	data_buffer[6] = (int)(MPU_Get_Pitch_Now() * 10000) & 0xff;
	data_buffer[7] = ((int)(MPU_Get_Pitch_Now() * 10000) >> 8) & 0xff;
	data_buffer[8] = (int)(MPU_Get_Yaw_Now() * 10000) & 0xff;
	data_buffer[9] = ((int)(MPU_Get_Yaw_Now() * 10000) >> 8) & 0xff;

	for (i = 2; i < Roll_Pitch_LEN - 1; i++)
	{
		checknum += data_buffer[i];
	}
	data_buffer[Roll_Pitch_LEN - 1] = checknum;
	usartdmasend(data_buffer, sizeof(data_buffer));
}

void timer1_Callback(void)
{
	MPU9250_Read_Data_Handle();
	//   printf("enter timer1_Callback.......\n");
}
osTimerId_t Timer_ID; // 定时器ID
void soft_timer1_init()
{
	Timer_ID = osTimerNew(timer1_Callback, osTimerPeriodic, NULL, NULL); // 创建定时器
	if (Timer_ID != NULL)
	{
		printf("ID = %d, Create Timer_ID is OK!\n", Timer_ID);

		osStatus_t timerStatus = osTimerStart(Timer_ID, 10U); // 开始定时器， 并赋予定时器的定时值
		if (timerStatus != osOK)
		{
			printf("timer is not startRun !\n");
		}
	}
}