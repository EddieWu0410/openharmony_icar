#include "bsp_iic_soft.h"

// 初始化软件IIC
// SCL  开漏输出模式
// SDA  开漏输出模式
void bsp_iic_soft_config()
{
	// SCL
	crm_periph_clock_enable(SCL_CLK, TRUE); // 开启SCL的时钟
	gpio_init_type gpio_init_struct;
	gpio_default_para_init(&gpio_init_struct);
	gpio_init_struct.gpio_pins = SCL_PIN;
	gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT; /* 选择输出*/
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE;   /* 无、上拉或下拉 */
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init(SCL_GPIO, &gpio_init_struct);
	// SDA
	crm_periph_clock_enable(SDA_CLK, TRUE); // 开启SDA的时钟
	gpio_default_para_init(&gpio_init_struct);
	gpio_init_struct.gpio_pins = SDA_PIN;
	gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE; /* 无、上拉或下拉 */
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init(SDA_GPIO, &gpio_init_struct);
}

void SDA_MODE_SET(uint8_t __mode)
{
	gpio_init_type gpio_init_struct;
	crm_periph_clock_enable(SDA_CLK, TRUE); // 开启SDA的时钟
	gpio_default_para_init(&gpio_init_struct);
	gpio_init_struct.gpio_pins = SDA_PIN;
	gpio_init_struct.gpio_mode = __mode;
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE; /* 无、上拉或下拉 */
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init(SDA_GPIO, &gpio_init_struct);
}
/*******************************************IIC信号***********************************************/
// IIC起始信号
void MPU_IIC_Start(void)
{
	SDA_OUT();
	SDA(1);
	SCL(1);
	soft_iic_delay(4);
	SDA(0);
	soft_iic_delay(4);
	SCL(0);
}
// IIC停止信号
void MPU_IIC_Stop(void)
{
	SDA_OUT();
	SCL(0);
	SDA(0);
	soft_iic_delay(4);
	SCL(1);
	SDA(1);
	soft_iic_delay(4);
}
// 等待从机应答 0：接收应答成功  1：接收应答失败
uint8_t MPU_IIC_Wait_Ack(void)
{
	u8 ucErrTime = 0;
	SDA_IN();
	SDA(1);
	soft_iic_delay(1);
	SCL(1);
	soft_iic_delay(1);
	while (SDA_GET==1)
	{
		ucErrTime++;
		if (ucErrTime > 250)
		{
			MPU_IIC_Stop();
			return 1;
		}
	}
	SCL(0);
	return 0;
}
// 产生ACK应答
void MPU_IIC_Ack(void)
{
	SCL(0);
	SDA_OUT();
	SDA(0);
	soft_iic_delay(2);
	SCL(1);
	soft_iic_delay(2);
	SCL(0);
}
// 不产生ACK应答
void MPU_IIC_NAck(void)
{
	SCL(0);
	SDA_OUT();
	SDA(1);
	soft_iic_delay(2);
	SCL(1);
	soft_iic_delay(2);
	SCL(0);
}
/*******************************************IIC读写1byte***********************************************/
// IIC写一个字节
void MPU_IIC_Send_Byte(uint8_t data)
{
	int i = 0;
	SDA_OUT();
	SCL(0); // 拉低时钟开始数据传输

	for (i = 0; i < 8; i++)
	{
		SDA((data & 0x80) >> 7);
		data <<= 1;
		soft_iic_delay(2);
		SCL(1);
		soft_iic_delay(2);
		SCL(0);
		soft_iic_delay(2);
	}
}
// IIC读一个字节 ack=1时，发送ACK，ack=0，发送nACK
uint8_t MPU_IIC_Read_Byte(unsigned char ack)
{
	unsigned char i, receive = 0;
	SDA_IN(); // SDA设置为输入
	for (i = 0; i < 8; i++)
	{
		SCL(0);
		soft_iic_delay(2);
		SCL(1);
		receive <<= 1;
		if (SDA_GET)
		{
			receive++;
		}
		soft_iic_delay(1);
	}
	if (!ack)
		MPU_IIC_NAck(); // 发送nACK
	else
		MPU_IIC_Ack(); // 发送ACK
	return receive;
}
// IIC连续写, 返回值:0,正常 , 其他,错误代码
uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	uint8_t i;
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 0); //发送器件地址+写命令
	if (MPU_IIC_Wait_Ack())				//等待应答
	{
		MPU_IIC_Stop();
		return 1;
	}
	MPU_IIC_Send_Byte(reg); //写寄存器地址
	MPU_IIC_Wait_Ack();		//等待应答
	for (i = 0; i < len; i++)
	{
		MPU_IIC_Send_Byte(buf[i]); //发送数据
		if (MPU_IIC_Wait_Ack())	   //等待ACK
		{
			MPU_IIC_Stop();
			return 1;
		}
	}
	MPU_IIC_Stop();
	return 0;
}

// IIC连续读, 返回值:0,正常 , 其他,错误代码
uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 0); //发送器件地址+写命令
	if (MPU_IIC_Wait_Ack())				//等待应答
	{
		MPU_IIC_Stop();
		return 1;
	}
	MPU_IIC_Send_Byte(reg); //写寄存器地址
	MPU_IIC_Wait_Ack();		//等待应答
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 1); //发送器件地址+读命令
	MPU_IIC_Wait_Ack();					//等待应答
	while (len)
	{
		if (len == 1)
			*buf = MPU_IIC_Read_Byte(0); //读数据,发送nACK
		else
			*buf = MPU_IIC_Read_Byte(1); //读数据,发送ACK
		len--;
		buf++;
	}
	MPU_IIC_Stop(); //产生一个停止条件
	return 0;
}

// IIC写一个字节, 返回值:0,正常 , 其他,错误代码
uint8_t MPU_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data)
{
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 0); //发送器件地址+写命令
	if (MPU_IIC_Wait_Ack())				//等待应答
	{
		MPU_IIC_Stop();
		return 1;
	}
	MPU_IIC_Send_Byte(reg);	 //写寄存器地址
	MPU_IIC_Wait_Ack();		 //等待应答
	MPU_IIC_Send_Byte(data); //发送数据
	if (MPU_IIC_Wait_Ack())	 //等待ACK
	{
		MPU_IIC_Stop();
		return 1;
	}
	MPU_IIC_Stop();
	return 0;
}

// IIC读一个字节, 返回值:0,正常 , 其他,错误代码
uint8_t MPU_Read_Byte(uint8_t addr, uint8_t reg)
{
	uint8_t res;
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 0); //发送器件地址+写命令
	MPU_IIC_Wait_Ack();					//等待应答
	MPU_IIC_Send_Byte(reg);				//写寄存器地址
	MPU_IIC_Wait_Ack();					//等待应答
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 1); //发送器件地址+读命令
	MPU_IIC_Wait_Ack();					//等待应答
	res = MPU_IIC_Read_Byte(0);			//读数据,发送nACK
	MPU_IIC_Stop();						//产生一个停止条件
	return res;
}

void MPU_Scanf_Addr(void)
{
    uint8_t i2c_count = 0;
	MPU_IIC_Init();
	MPU_ADDR_CTRL();
    for (int i = 1; i < 128; i++)
    {
        MPU_IIC_Start();
        MPU_IIC_Send_Byte(i << 1);
        if (MPU_IIC_Wait_Ack())
        {
            if (i == 127)
            {
                printf("MPU IIC Scanf End, Count=%d\n", i2c_count);
            }
            continue;
        }
        MPU_IIC_Stop();
        i2c_count++;
        printf("MPU IIC Found address:0x%2x\n", i);
    }
}
