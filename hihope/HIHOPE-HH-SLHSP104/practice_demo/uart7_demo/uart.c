#include "uart.h"

void USART1_init(uint32_t bound)
{
    gpio_init_type gpio_init_struct;
    /*Enable the USART1 Clock*/
    crm_periph_clock_enable(CRM_GPIOE_PERIPH_CLOCK, TRUE); // 开启GPIOE的时钟
    crm_periph_clock_enable(CRM_USART1_PERIPH_CLOCK, TRUE); // 开启USART1的时钟

    gpio_default_para_init(&gpio_init_struct);
    /* Configure the USART1 TX pin */
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER; // 较大电流推动/吸入能力
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;              // 推挽输出
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;                          // 复用
    gpio_init_struct.gpio_pins = GPIO_PINS_8;                            // PE8
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;                         // 无上下拉
    gpio_init(GPIOE, &gpio_init_struct);

    /* Configure the USART1 RX pin */
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER; // 较大电流推动/吸入能力
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;              // 推挽输出
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;                        // 输入模式
    gpio_init_struct.gpio_pins = GPIO_PINS_7;                            // PE7
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;                           // 上拉
    gpio_init(GPIOE, &gpio_init_struct);

    nvic_irq_enable(USART1_IRQn, 0, 6); // 使能串口7中断，优先级0，次优先级6

    /*Configure UART param*/
    usart_init(USART1, bound, USART_DATA_8BITS, USART_STOP_1_BIT);     // 波特率，8数据位，1停止位
    usart_hardware_flow_control_set(USART1, USART_HARDWARE_FLOW_NONE); // 无硬件流操作
    usart_parity_selection_config(USART1, USART_PARITY_NONE);          // 无校验
    usart_transmitter_enable(USART1, TRUE);                            // 使能发送
    usart_receiver_enable(USART1, TRUE);                               // 使能接收

    usart_interrupt_enable(USART1, USART_RDBF_INT, TRUE); // 使能串口接收中断
    usart_interrupt_enable(USART1, USART_IDLE_INT, TRUE); // 使能串口空闲中断
    usart_enable(USART1, TRUE);                           // 使能串口
}


/**
 *	串口7的中断函数
 */
#define RX_BUFFER_LEN 1024
uint8_t g_rx_buffer[RX_BUFFER_LEN];
uint32_t g_rx_cnt = 0;
void USART1_IRQHandler(void)
{
    uint8_t data;
    if (usart_flag_get(USART1, USART_RDBF_FLAG) != RESET)
    {
        usart_flag_clear(USART1, USART_RDBF_FLAG);
        // 获取寄存器里的数据, 缓存到buffer中
         g_rx_buffer[g_rx_cnt++] = usart_data_receive(USART1);
        // 避免缓冲区溢出
        if (g_rx_cnt > RX_BUFFER_LEN)
        {
            g_rx_cnt = 0;
        }
    }
    else if (usart_flag_get(USART1, USART_IDLEF_FLAG) != RESET)
    {
        usart_data_receive(USART1);
        // 添加字符串结束标记，避免打印出错
        g_rx_buffer[g_rx_cnt] = '\0';
#if USART7_RECV_CALLBACK
        Usart7_on_recv(g_rx_buffer, g_rx_cnt);
#endif
        // 重置缓冲区数据个数
        g_rx_cnt = 0;
        usart_flag_clear(USART1, USART_IDLEF_FLAG);
    }
}


// 发送1个byte数据
void USART7_send_byte(uint8_t byte)
{
    while (RESET == usart_flag_get(UART7, USART_TDBE_FLAG))
        ;
    // 从USART7的TX发送一个字节出去
    usart_data_transmit(USART1, (uint8_t)byte);
    // 等待发送完成 (轮询等待发送数据缓冲区为空)
    while (RESET == usart_flag_get(UART7, USART_TDC_FLAG))
        ;
}

// 发送多个byte数据
void USART7_send_data(uint8_t *data, uint32_t len)
{
    // 满足：1.data指针不为空 2.长度不为0
    while (data && len--)
    {
        USART7_send_byte(*data);
        data++;
    }
}

// 发送字符串
void USART7_send_string(uint8_t *data)
{
    for (; *data != 0; data++)
    {
        USART7_send_byte(*data);
    }
}
