#ifndef _BSP_IIC_SOFT_H_
#define _BSP_IIC_SOFT_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_board.h"

#define u8 uint8_t
#define u16 uint16_t

// SCL和SDA引脚
#define SCL_CLK CRM_GPIOE_PERIPH_CLOCK
#define SCL_GPIO GPIOE
#define SCL_PIN GPIO_PINS_11
#define SDA_CLK CRM_GPIOE_PERIPH_CLOCK
#define SDA_GPIO GPIOE
#define SDA_PIN GPIO_PINS_14


#define SDA_OUT() {SDA_MODE_SET( GPIO_MODE_OUTPUT );}  //SDA输出模式
#define SDA_IN()  {SDA_MODE_SET( GPIO_MODE_INPUT  );}  //SDA输入模式

// SCL和SDA高低电平
#define SCL(BIT) gpio_bits_write(SCL_GPIO, SCL_PIN,BIT)
#define SDA(BIT) gpio_bits_write(SDA_GPIO, SDA_PIN,BIT)

// 获取SDA电平
#define SDA_GET gpio_input_data_bit_read(SDA_GPIO, SDA_PIN)

// 延时
#define soft_iic_delay(a) dwt_delay_us(a)

// 初始化
void bsp_iic_soft_config();
//MPU 函数
uint8_t MPU_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data);
uint8_t MPU_Read_Byte(uint8_t addr, uint8_t reg);
uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf);
uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf);
void MPU_Scanf_Addr(void);

#endif