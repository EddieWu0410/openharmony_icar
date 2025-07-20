#ifndef __TEM_HUM_H__   // 判断是否已经定义过 __TEM_HUM_H__，避免重复包含
#define __TEM_HUM_H__   // 定义 __TEM_HUM_H__，标志该文件已被包含

#include "cmsis_os2.h"      // 引入 CMSIS RTOS2 相关头文件，用于 RTOS 操作系统功能
#include "ohos_init.h"       // 引入 OpenHarmony 初始化相关的头文件
#include "at32f403a_407_gpio.h"  // 引入 AT32 微控制器的 GPIO（通用输入输出）控制相关头文件
#include "at32f403a_407_crm.h"   // 引入 AT32 微控制器的 CRM（时钟与复位管理）相关头文件
#include "at32f403a_407_i2c.h"   // 引入 AT32 微控制器的 I2C 总线控制相关头文件
#include "i2c_application.h"      // 引入自定义的 I2C 应用层代码头文件，用于 I2C 总线通信

// 定义 AHT20 温湿度传感器的 I2C 配置和地址
// 定义使用的 I2C 总线索引，此处使用 I2C 总线 2
#define AHT20_I2C_IDX       (2)
// 定义 AHT20 温湿度传感器的设备地址，7位设备地址为 0x38
#define AHT20_DEVICE_ADDR   (0x38)
// 定义 AHT20 传感器的读地址，I2C 地址左移一位并加上读操作标志 1
#define AHT20_READ_ADDR     ((0x38<<1)|0x1)
// 定义 AHT20 传感器的写地址，I2C 地址左移一位并加上写操作标志 0
#define AHT20_WRITE_ADDR    ((0x38<<1)|0x0) 

// 声明 I2C 句柄，用于与 I2C 总线进行交互
extern i2c_handle_type hi2cx;

// 函数声明：初始化温湿度传感器
void tem_hum_init(void);

// 函数声明：获取温湿度数据，并返回温度和湿度值
// 参数 temp: 用于接收温度数据（单位：°C）
// 参数 humi: 用于接收湿度数据（单位：%）
float tem_hum_get_data(float *temp, float *humi);

#endif             // 结束条件编译
