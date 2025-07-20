#include "at32f403a_407.h"
#include "usart3.h"
#include "string.h"
/****************************************************************************************************/

uint8_t Serial3_RxPacket[20] ={0};	

/****************************************************************************************************/

//消息队列的部分
osMessageQueueId_t uart3_recv_queue={0};
uint8_t Serial3_data[Packet_size]={0};

/****************************************************************************************************/

//usart3初始化
void ctrl_uart3_init(void)
{
  gpio_init_type gpio_init_struct;

  /* enable iomux periph clock */
  crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
  crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);
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
  gpio_init_struct.gpio_pull = GPIO_PULL_UP;								//上拉
  gpio_init(GPIOD, &gpio_init_struct);

  // 开启重映射，设置USART3的引脚为PD8和PD9
  gpio_pin_remap_config(USART3_GMUX_0011, TRUE);

  usart_init(USART3, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);
  usart_transmitter_enable(USART3, TRUE);
  usart_receiver_enable(USART3, TRUE);
  usart_parity_selection_config(USART3, USART_PARITY_NONE);

  // nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
  nvic_irq_enable(USART3_IRQn, 1, 2);

  usart_interrupt_enable(USART3, USART_RDBF_INT, TRUE);
  usart_interrupt_enable(USART3, USART_IDLE_INT, TRUE);
  usart_enable(USART3, TRUE);

  // while (usart_flag_get(USART3, USART_TDBE_FLAG) == RESET);
}
/****************************************************************************************************/
static volatile  uint8_t Serial3_RxPacket_recv[20];	
static uint8_t pRx3Packet = 0;	//定义表示当前接收数据位置的静态变量
static uint8_t Rx3State = 0;		//定义表示当前状态机状态的静态变量
/**
 * 函    数：USART3中断函数
 * 参    数：无
 * 返 回 值：无
 * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
 *           函数名为预留的指定名称，可以从启动文件复制
 *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
 */
// device/soc/artery/at32f4xx/liteos_m/startup/startup_at32f403a_407.s里面含有USART3_IRQHandler
//中断服务程序里，不能调用printf,malloc 函数。
void USART3_IRQHandler(void)
{
	
	if (usart_flag_get(USART3, USART_RDBF_FLAG) == SET)		//判断是否是USART1的接收事件触发的中断
	{
		uint8_t RxData = usart_data_receive(USART3);
		Serial3_RxPacket_recv[pRx3Packet++] = RxData;
		
		// usart_data_transmit(USART1, usart_flag_get(USART1, USART_RDBF_FLAG));
		usart_flag_clear(USART3, USART_RDBF_FLAG);
		while(usart_flag_get(USART3, USART_TDBE_FLAG) == RESET);
	}
	else if(usart_flag_get(USART3, USART_IDLEF_FLAG) == SET)
	{
		int lenght = Serial3_RxPacket_recv[2];
		//处理数据
		if(Serial3_RxPacket_recv[0]==0xFF && Serial3_RxPacket_recv[1]==0xFC )
		{
			for(int i=0;i<lenght;i++)
			{
				Serial3_data[i]=Serial3_RxPacket_recv[i+2];
			}
		}
		
		osMessageQueuePut (uart3_recv_queue, &Serial3_data, 0, 0);
		USART3->sts; 
		USART3->dt;
		pRx3Packet=0;
		memset(Serial3_RxPacket_recv,0,20);
		while(usart_flag_get(USART3, USART_TDBE_FLAG) == RESET);
	}


}

/****************************************************************************************************/

/**
  * 函    数：串口发送一个字节
  * 参    数：Byte 要发送的一个字节
  * 返 回 值：无
  */
void Serial3_SendByte(uint8_t byte)
{
  while(usart_flag_get(USART3, USART_TDBE_FLAG) == RESET);
  usart_data_transmit(USART3, byte);
   while(usart_flag_get(USART3, USART_TDC_FLAG) == RESET);
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
	for (i = 0; i < Length; i ++)		//遍历数组
	{
		Serial_SendByte(Array[i]);		//依次调用Serial_SendByte发送每个字节数据
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
	// for (i = 0; String[i] != '\0'; i++)//遍历字符数组（字符串），遇到字符串结束标志位后停止
	// {
	// 	Serial3_SendByte(String[i]);		//依次调用Serial_SendByte发送每个字节数据
	// }
  for (i = 0; i<strlen(String); i++)//遍历字符数组（字符串），遇到字符串结束标志位后停止
	{
		Serial3_SendByte(String[i]);		//依次调用Serial_SendByte发送每个字节数据
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
	for (i = 0; i < Length; i ++)		//根据数字长度遍历数字的每一位
	{
		Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0');	//依次调用Serial_SendByte发送每位数字
	}
}

/*********************************************************************************************************/

