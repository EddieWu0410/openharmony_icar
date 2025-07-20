#include "at32f403a_407.h"
#include "usart1.h"

#include "devmgr_service_start.h"
#include "los_task.h"
#include "los_memory.h"
#include "cmsis_os2.h"
#include "at32f403a_407.h"
/****************************************************************************************************/
// #define MAX_LEN 256
uint8_t Serial1_RxPacket[Packet_size] = {0};

/****************************************************************************************************/

// 消息队列的部分
osMessageQueueId_t uart1_recv_queue = {0};
uint8_t Serial1_data[Packet_size] = {0};

/****************************************************************************************************/
void ctrl_uart1_init(void)
{
  gpio_init_type gpio_init_struct;
  dma_init_type dma_init_struct;

  /*Enable the UART Clock*/
  crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);  // 开启GPIOA的时钟
  crm_periph_clock_enable(CRM_USART1_PERIPH_CLOCK, TRUE); // 开启USART1的时钟
  crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK, TRUE);   // 开启DMA1的时钟

  gpio_default_para_init(&gpio_init_struct);
  /* Configure the UART1 TX pin */
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER; // 较大电流推动/吸入能力
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;              // 推挽输出
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;                          // 复用
  gpio_init_struct.gpio_pins = GPIO_PINS_9;                            // PA9
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;                         // 无上下拉
  gpio_init(GPIOA, &gpio_init_struct);

  /* Configure the UART1 RX pin */
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER; // 较大电流推动/吸入能力
  gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;              // 推挽输出（输入模式，无效）
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;                        // 输入模式
  gpio_init_struct.gpio_pins = GPIO_PINS_10;                           // PA10
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;                         // 无上下拉
  gpio_init(GPIOA, &gpio_init_struct);
  // tx
  dma_reset(DMA1_CHANNEL4);
  dma_default_para_init(&dma_init_struct);
  dma_init_struct.buffer_size = 0;                                        // 内存大小
  dma_init_struct.direction = DMA_DIR_MEMORY_TO_PERIPHERAL;               // 外设地址为目的地址
  dma_init_struct.memory_base_addr = (uint32_t)0;                         // 内存地址
  dma_init_struct.memory_data_width = DMA_MEMORY_DATA_WIDTH_BYTE;         // 内存数据的宽度
  dma_init_struct.memory_inc_enable = TRUE;                               // 内存地址递增打开
  dma_init_struct.peripheral_base_addr = (uint32_t)&USART1->dt;           // 外设地址
  dma_init_struct.peripheral_data_width = DMA_PERIPHERAL_DATA_WIDTH_BYTE; // 外设数据的宽度
  dma_init_struct.peripheral_inc_enable = FALSE;                          // 外设地址递增关闭
  dma_init_struct.priority = DMA_PRIORITY_HIGH;                           // 中等优先级
  dma_init_struct.loop_mode_enable = FALSE;                               // 不循环
  dma_init(DMA1_CHANNEL4, &dma_init_struct);
  // rx
  // dma_reset(DMA1_CHANNEL5);
  // dma_init_struct.peripheral_base_addr = (uint32_t)&USART1->dt;  // 外设地址
  // dma_init_struct.memory_base_addr = (uint32_t)Serial1_RxPacket; // 内存地址
  // dma_init_struct.memory_inc_enable = TRUE;                      // 内存地址递增打开
  // dma_init_struct.direction = DMA_DIR_PERIPHERAL_TO_MEMORY;      // 外设地址为源地址
  // dma_init_struct.buffer_size = Packet_size;
  // dma_init_struct.peripheral_inc_enable = FALSE; // 外设地址递增
  // dma_init(DMA1_CHANNEL5, &dma_init_struct);

  // dma_channel_enable(DMA1_CHANNEL5, TRUE); // 使能通道5

  /*Configure UART param*/
  usart_init(USART1, 115200, USART_DATA_8BITS, USART_STOP_1_BIT);    // 波特率，8数据位，1停止位
  usart_hardware_flow_control_set(USART1, USART_HARDWARE_FLOW_NONE); // 无硬件流操作
  usart_transmitter_enable(USART1, TRUE);                            // 使能发送
  usart_receiver_enable(USART1, TRUE);                               // 使能接收
  usart_parity_selection_config(USART1, USART_PARITY_NONE);          // 无校验
  usart_dma_transmitter_enable(USART1, TRUE);
  // usart_dma_receiver_enable(USART1, TRUE); // 使能串口dma接收
  // nvic
  nvic_irq_enable(USART1_IRQn, 1, 3);                   // 使能串口1中断，优先级1，次优先级3
  // usart_interrupt_enable(USART1, USART_IDLE_INT, TRUE); // 使能串口空闲中断
  usart_interrupt_enable(USART1, USART_RDBF_INT, TRUE); // 接收中断
  usart_enable(USART1, TRUE);                           // 使能串口
  usart_flag_clear(USART1, USART_TDC_FLAG);
}
/****************************************************************************************************/

/**
 * 函    数：串口发送一个字节
 * 参    数：Byte 要发送的一个字节
 * 返 回 值：无
 */
void Serial1_SendByte(uint8_t byte)
{
  while (usart_flag_get(USART1, USART_TDBE_FLAG) == RESET)
    ;
  usart_data_transmit(USART1, byte);
  while (usart_flag_get(USART1, USART_TDC_FLAG) == RESET)
    ;
}

/**
 * 函    数：串口发送一个数组
 * 参    数：Array 要发送数组的首地址
 * 参    数：Length 要发送数组的长度
 * 返 回 值：无
 */
void Serial1_SendArray(uint8_t *Array, uint16_t Length)
{
  uint16_t i;
  for (i = 0; i < Length; i++) // 遍历数组
  {
    Serial1_SendByte(Array[i]); // 依次调用Serial_SendByte发送每个字节数据
  }
}

/*********************************************************************************************************/

/**
 * 函    数：串口发送一个字符串
 * 参    数：String 要发送字符串的首地址
 * 返 回 值：无
 */
void Serial1_SendString(char *String)
{
  uint8_t i;
  for (i = 0; String[i] != '\0'; i++) // 遍历字符数组（字符串），遇到字符串结束标志位后停止
  {
    Serial1_SendByte(String[i]); // 依次调用Serial_SendByte发送每个字节数据
  }
}

/*********************************************************************************************************/

/**
 * 函    数：串口发送数字
 * 参    数：Number 要发送的数字，范围：0~4294967295
 * 参    数：Length 要发送数字的长度，范围：0~10
 * 返 回 值：无
 */
void Serial1_SendNumber(uint32_t Number, uint8_t Length)
{
  uint8_t i;
  for (i = 0; i < Length; i++) // 根据数字长度遍历数字的每一位
  {
    Serial1_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0'); // 依次调用Serial_SendByte发送每位数字
  }
}

/*********************************************************************************************************/
static uint32_t rx_cnt = 0;
void USART1_IRQHandler(void)
{
  uint8_t data = 0;
  if (usart_flag_get(USART1, USART_RDBF_FLAG) != RESET)
  {
    data = usart_data_receive(USART1);
    Upper_Data_Receive(data);
  }
}
/*******************************************dma funtion***********************************************/
void Usart1_dma_channel_config(dma_channel_type *dmax_channely, uint32_t peripheral_base_addr, uint32_t memory_base_addr, uint16_t buffer_size)
{
  dmax_channely->dtcnt = buffer_size;
  dmax_channely->paddr = peripheral_base_addr;
  dmax_channely->maddr = memory_base_addr;
}
void usartdmasend(uint8_t *buf, uint16_t len)
{
  Usart1_dma_channel_config(DMA1_CHANNEL4, (uint32_t)&USART1->dt, (uint32_t)buf, len);
  dma_channel_enable(DMA1_CHANNEL4, TRUE);
  while (dma_flag_get(DMA1_FDT4_FLAG) == RESET)
    ;
  dma_flag_clear(DMA1_FDT4_FLAG);
  dma_channel_enable(DMA1_CHANNEL4, FALSE);
}