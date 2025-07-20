#include "bsp_iic_soft.h"

// ��ʼ�����IIC
// SCL  ��©���ģʽ
// SDA  ��©���ģʽ
void bsp_iic_soft_config()
{
	// SCL
	crm_periph_clock_enable(SCL_CLK, TRUE); // ����SCL��ʱ��
	gpio_init_type gpio_init_struct;
	gpio_default_para_init(&gpio_init_struct);
	gpio_init_struct.gpio_pins = SCL_PIN;
	gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT; /* ѡ�����*/
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE;   /* �ޡ����������� */
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init(SCL_GPIO, &gpio_init_struct);
	// SDA
	crm_periph_clock_enable(SDA_CLK, TRUE); // ����SDA��ʱ��
	gpio_default_para_init(&gpio_init_struct);
	gpio_init_struct.gpio_pins = SDA_PIN;
	gpio_init_struct.gpio_mode = GPIO_MODE_OUTPUT;
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE; /* �ޡ����������� */
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init(SDA_GPIO, &gpio_init_struct);
}

void SDA_MODE_SET(uint8_t __mode)
{
	gpio_init_type gpio_init_struct;
	crm_periph_clock_enable(SDA_CLK, TRUE); // ����SDA��ʱ��
	gpio_default_para_init(&gpio_init_struct);
	gpio_init_struct.gpio_pins = SDA_PIN;
	gpio_init_struct.gpio_mode = __mode;
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE; /* �ޡ����������� */
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init(SDA_GPIO, &gpio_init_struct);
}
/*******************************************IIC�ź�***********************************************/
// IIC��ʼ�ź�
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
// IICֹͣ�ź�
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
// �ȴ��ӻ�Ӧ�� 0������Ӧ��ɹ�  1������Ӧ��ʧ��
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
// ����ACKӦ��
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
// ������ACKӦ��
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
/*******************************************IIC��д1byte***********************************************/
// IICдһ���ֽ�
void MPU_IIC_Send_Byte(uint8_t data)
{
	int i = 0;
	SDA_OUT();
	SCL(0); // ����ʱ�ӿ�ʼ���ݴ���

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
// IIC��һ���ֽ� ack=1ʱ������ACK��ack=0������nACK
uint8_t MPU_IIC_Read_Byte(unsigned char ack)
{
	unsigned char i, receive = 0;
	SDA_IN(); // SDA����Ϊ����
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
		MPU_IIC_NAck(); // ����nACK
	else
		MPU_IIC_Ack(); // ����ACK
	return receive;
}
// IIC����д, ����ֵ:0,���� , ����,�������
uint8_t MPU_Write_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	uint8_t i;
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 0); //����������ַ+д����
	if (MPU_IIC_Wait_Ack())				//�ȴ�Ӧ��
	{
		MPU_IIC_Stop();
		return 1;
	}
	MPU_IIC_Send_Byte(reg); //д�Ĵ�����ַ
	MPU_IIC_Wait_Ack();		//�ȴ�Ӧ��
	for (i = 0; i < len; i++)
	{
		MPU_IIC_Send_Byte(buf[i]); //��������
		if (MPU_IIC_Wait_Ack())	   //�ȴ�ACK
		{
			MPU_IIC_Stop();
			return 1;
		}
	}
	MPU_IIC_Stop();
	return 0;
}

// IIC������, ����ֵ:0,���� , ����,�������
uint8_t MPU_Read_Len(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 0); //����������ַ+д����
	if (MPU_IIC_Wait_Ack())				//�ȴ�Ӧ��
	{
		MPU_IIC_Stop();
		return 1;
	}
	MPU_IIC_Send_Byte(reg); //д�Ĵ�����ַ
	MPU_IIC_Wait_Ack();		//�ȴ�Ӧ��
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 1); //����������ַ+������
	MPU_IIC_Wait_Ack();					//�ȴ�Ӧ��
	while (len)
	{
		if (len == 1)
			*buf = MPU_IIC_Read_Byte(0); //������,����nACK
		else
			*buf = MPU_IIC_Read_Byte(1); //������,����ACK
		len--;
		buf++;
	}
	MPU_IIC_Stop(); //����һ��ֹͣ����
	return 0;
}

// IICдһ���ֽ�, ����ֵ:0,���� , ����,�������
uint8_t MPU_Write_Byte(uint8_t addr, uint8_t reg, uint8_t data)
{
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 0); //����������ַ+д����
	if (MPU_IIC_Wait_Ack())				//�ȴ�Ӧ��
	{
		MPU_IIC_Stop();
		return 1;
	}
	MPU_IIC_Send_Byte(reg);	 //д�Ĵ�����ַ
	MPU_IIC_Wait_Ack();		 //�ȴ�Ӧ��
	MPU_IIC_Send_Byte(data); //��������
	if (MPU_IIC_Wait_Ack())	 //�ȴ�ACK
	{
		MPU_IIC_Stop();
		return 1;
	}
	MPU_IIC_Stop();
	return 0;
}

// IIC��һ���ֽ�, ����ֵ:0,���� , ����,�������
uint8_t MPU_Read_Byte(uint8_t addr, uint8_t reg)
{
	uint8_t res;
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 0); //����������ַ+д����
	MPU_IIC_Wait_Ack();					//�ȴ�Ӧ��
	MPU_IIC_Send_Byte(reg);				//д�Ĵ�����ַ
	MPU_IIC_Wait_Ack();					//�ȴ�Ӧ��
	MPU_IIC_Start();
	MPU_IIC_Send_Byte((addr << 1) | 1); //����������ַ+������
	MPU_IIC_Wait_Ack();					//�ȴ�Ӧ��
	res = MPU_IIC_Read_Byte(0);			//������,����nACK
	MPU_IIC_Stop();						//����һ��ֹͣ����
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
