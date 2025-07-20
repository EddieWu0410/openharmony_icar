#include "usart3.h"
// usart3初始化
void bsp_uart3_init(uint32_t baud)
{
	gpio_init_type gpio_init_struct;

	/* enable iomux periph clock */
	crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);

	/* enable gpiod periph clock */
	crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);

	/* enable usart3 periph clock */
	crm_periph_clock_enable(CRM_USART3_PERIPH_CLOCK, TRUE);

	gpio_default_para_init(&gpio_init_struct);

	/* configure the TX pin */
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
	gpio_init_struct.gpio_pins = GPIO_PINS_8;
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
	gpio_init(GPIOD, &gpio_init_struct);

	/* configure the RX pin */
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
	gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
	gpio_init_struct.gpio_pins = GPIO_PINS_9;
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
	gpio_init(GPIOD, &gpio_init_struct);

	// 开启重映射，设置USART3的引脚为PD8和PD9
	gpio_pin_remap_config(USART3_GMUX_0011, TRUE);

	usart_init(USART3, baud, USART_DATA_8BITS, USART_STOP_1_BIT);
	usart_transmitter_enable(USART3, TRUE);
	usart_receiver_enable(USART3, TRUE);
	usart_parity_selection_config(USART3, USART_PARITY_NONE);

	// nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
	nvic_irq_enable(USART3_IRQn, 1, 2);

	usart_interrupt_enable(USART3, USART_RDBF_INT, TRUE);
	usart_interrupt_enable(USART3, USART_IDLE_INT, TRUE);
	usart_enable(USART3, TRUE);

	while (usart_flag_get(USART3, USART_TDBE_FLAG) == RESET)
		;
}

/****************************************************************************************************/

/**
 * 函    数：串口发送一个字节
 * 参    数：Byte 要发送的一个字节
 * 返 回 值：无
 */
void Serial3_SendByte(uint8_t byte)
{
	while (usart_flag_get(USART3, USART_TDBE_FLAG) == RESET);
	usart_data_transmit(USART3, byte);
}

/**
 * 函    数：串口发送一个数组
 * 参    数：Array 要发送数组的首地址
 * 参    数：Length 要发送数组的长度
 * 返 回 值：无
 */
void Serial3_SendArray(uint8_t *Array, uint16_t Length)
{
	uint16_t i;
	for (i = 0; i < Length; i++) // 遍历数组
	{
		Serial3_SendByte(Array[i]); // 依次调用Serial3_SendByte发送每个字节数据
	}
}

/*********************************************************************************************************/

/**
 * 函    数：串口发送一个字符串
 * 参    数：String 要发送字符串的首地址
 * 返 回 值：无
 */
void Serial3_SendString(char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++) // 遍历字符数组（字符串），遇到字符串结束标志位后停止
	{
		Serial3_SendByte(String[i]); // 依次调用Serial3_SendByte发送每个字节数据
	}
}

/*********************************************************************************************************/

/**
 * 函    数：串口发送数字
 * 参    数：Number 要发送的数字，范围：0~4294967295
 * 参    数：Length 要发送数字的长度，范围：0~10
 * 返 回 值：无
 */
void Serial3_SendNumber(uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++) // 根据数字长度遍历数字的每一位
	{
		Serial3_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0'); // 依次调用Serial3_SendByte发送每位数字
	}
}


void USART3_IRQHandler(void)
{
  uint8_t data = 0;
  if (usart_flag_get(USART1, USART_RDBF_FLAG) != RESET)
  {
    data = usart_data_receive(USART3);
    Upper_Data_Receive(data);
  }
}