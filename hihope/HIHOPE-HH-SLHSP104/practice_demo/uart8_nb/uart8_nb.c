#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "cmsis_os2.h"
#include "ohos_init.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_usart.h"
#include "uart8_nb.h"

char g_rx_buffer[RX_BUFFER_LEN_MAX];
uint32_t g_rx_cnt = 0;                // RX缓存池当前长度
unsigned char NB_RX_FLAG = 0;         // 接收到数据标志
char tmpbuf[RX_BUFFER_LEN_MAX] = {0}; /*transform to hex */
/**
 * @brief  UART8初始化
 * @param  bound:波特率
 * @retval 无
 */
void uart8_init(uint32_t bound)
{
    gpio_init_type gpio_init_struct;
    /*Enable the UART8 Clock*/
    crm_periph_clock_enable(GPIO_CLK, TRUE); // 开启GPIOE的时钟
    crm_periph_clock_enable(UART_CLK, TRUE); // 开启UART8的时钟

    gpio_default_para_init(&gpio_init_struct);
    /* Configure the UART8 TX   pin */
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER; // 较大电流推动/吸入能力
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;              // 推挽输出
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;                          // 复用
    gpio_init_struct.gpio_pins = PIN_TX;                                 // PE1
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;                         // 无上下拉
    gpio_init(GPIO_TX, &gpio_init_struct);

    /* Configure the UART8 RX   pin */
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER; // 较大电流推动/吸入能力
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;              // 推挽输出
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;                        // 输入模式
    gpio_init_struct.gpio_pins = PIN_RX;                                 // PE0
    gpio_init_struct.gpio_pull = GPIO_PULL_UP;                           // 上拉
    gpio_init(GPIO_RX, &gpio_init_struct);

    nvic_irq_enable(UART_NUM_IRQn, 0, 7); // 使能串口1中断，优先级0，次优先级7

    /*Configure UART param*/
    usart_init(UART_NUM, bound, USART_DATA_8BITS, USART_STOP_1_BIT);     // 波特率，8数据位，1停止位
    usart_hardware_flow_control_set(UART_NUM, USART_HARDWARE_FLOW_NONE); // 无硬件流操作
    usart_parity_selection_config(UART_NUM, USART_PARITY_NONE);          // 无校验
    usart_transmitter_enable(UART_NUM, TRUE);                            // 使能发送
    usart_receiver_enable(UART_NUM, TRUE);                               // 使能接收

    usart_interrupt_enable(UART_NUM, USART_RDBF_INT, TRUE); // 使能串口接收中断
    usart_interrupt_enable(UART_NUM, USART_IDLE_INT, TRUE); // 使能串口空闲中断
    usart_enable(UART_NUM, TRUE);                           // 使能串口
}

/**
 *	串口8的中断函数
 */
void UART_NUM_IRQ_FUN()
{
    if (usart_flag_get(UART_NUM, USART_RDBF_FLAG) != RESET)
    {
        // usart_flag_clear(UART_NUM, USART_RDBF_FLAG);
        // 获取寄存器里的数据缓存到buffer中
        g_rx_buffer[g_rx_cnt] = usart_data_receive(UART_NUM);

#if (recv_view == 1)
        // usart_data_transmit(UART_NUM, g_rx_buffer[g_rx_cnt]);
        printf("%c", g_rx_buffer[g_rx_cnt]);
#endif
        // 避免缓冲区溢出
        g_rx_cnt = (g_rx_cnt + 1) % RX_BUFFER_LEN_MAX;
    }
    if (usart_flag_get(UART_NUM, USART_IDLEF_FLAG) != RESET)
    {
        usart_data_receive(UART_NUM); // 清除IDLEF标志位
        // 添加字符串结束标记，避免打印出错
        g_rx_buffer[g_rx_cnt] = '\0';
        // // 重置缓冲区数据个数
        // g_rx_cnt = 0;
        // 通知外部程序有数据
        NB_RX_FLAG = 1;
        // usart_flag_clear(UART_NUM, USART_IDLEF_FLAG);
    }
}

void USART8_send_byte(uint8_t byte)
{
    while (RESET == usart_flag_get(UART_NUM, USART_TDBE_FLAG))
        ;
    // 从USART7的TX发送一个字节出去
    usart_data_transmit(UART_NUM, (uint8_t)byte);
    // 等待发送完成 (轮询等待发送数据缓冲区为空)
    while (RESET == usart_flag_get(UART_NUM, USART_TDC_FLAG))
        ;
}
// 发送字符串
void USART8_send_string(uint8_t *data)
{
    for (; *data != 0; data++)
    {
        USART8_send_byte(*data);
    }
}
// 清除串口接收的数据
void USART8_clear_rx(void)
{
    unsigned int i = RX_BUFFER_LEN_MAX - 1;
    while (i)
    {
        g_rx_buffer[i--] = 0;
    }
    g_rx_cnt = 0;
    NB_RX_FLAG = 0;
}
/**
 * @brief  向MN316模块发送指令，并查看MN316模块是否返回想要的数据
 * @param  cmd:发送的AT指令
 * @param  ack:期望得到的应答
 * @param  waitms:等待时间
 * @param  cnt:重发次数
 * @retval ATOK=得到了想要的应答    AT_ERROR=没有得到想要的应答
 */
int NB_Send_Cmd(char *cmd, char *ack, unsigned int waitms)
{
    // 创建一个临时缓冲区来存储带有 "\r\n" 的指令
    char cmd_with_ending[1024]; // 确保数组足够大以容纳cmd + "\r\n"
    // 拼接命令和 "\r\n"
    int written = snprintf(cmd_with_ending, sizeof(cmd_with_ending), "%s\r\n", cmd);

    // 检查是否有截断
    if (written >= sizeof(cmd_with_ending))
    {
        printf("Warning: Command was truncated\n");
    }
    else
    {
        printf("Command: %s", cmd_with_ending);
    }

    USART8_send_string((unsigned char *)cmd_with_ending);
    // 发送AT指令
    osDelay(waitms);
    if (strstr((char *)g_rx_buffer, ack) != NULL)
    {
        printf("OK\n");
        return AT_OK;
    }
    else
    {
        return AT_ERROR;
    }
    USART8_clear_rx();
}

void nb_mqtt_init()
{
    uint8_t ret = 0;
    do
    {
        ret = NB_Send_Cmd("AT", "OK", 20);
    } while (ret == AT_ERROR); // 检查模块是否存在
    ret = NB_Send_Cmd("AT+MQTTDISC", "OK", 20); // 注销登录
    ret = NB_Send_Cmd("AT+MQTTDEL", "OK", 20);  // 删除通讯
    ret = NB_Send_Cmd("AT+CFUN=1", "OK", 20);
    ret = NB_Send_Cmd("AT+CIMI", "OK", 20);
    do
    {
        ret = NB_Send_Cmd("AT+CIMI", "460", 20);
    } while (ret == AT_ERROR); // 获取卡号,460，表明识别到卡了
    ret = NB_Send_Cmd("AT+CGATT=1", "OK", 20); // 激活网络，PDP
    do
    {
        ret = NB_Send_Cmd("AT+CGATT?", "+CGATT:1", 20);
    } while (ret == AT_ERROR); // 返回1,表明注网成功
    ret = NB_Send_Cmd("AT+CSQ", "OK", 20);
}

// 模块连接服务器
int nb_mqtt_conn(char *brokerAddress, uint16_t port, char *clientID, char *userName, char *password)
{
    uint8_t ret = 0;
    char buf[1024] = {0};

    /*AT+MQTTCFG="123.207.210.43",1883,"clientExample",60,"","",1,0 */
    if (userName != NULL && password != NULL)
    {
        snprintf(buf, sizeof(buf), "AT+MQTTCFG=\"%s\",%d,\"%s\",60,\"%s\",\"%s\",1,0",
                 brokerAddress, port,
                 clientID, userName, password);
    }
    do
    {
        ret = NB_Send_Cmd(buf, "OK", 20);
    } while (ret == AT_ERROR);
    if (userName != NULL && password != NULL)
    {
        memset(buf, 0, sizeof(buf));
        snprintf(buf, sizeof(buf), "AT+MQTTOPEN=1,1,0,0,0,\"\",\"\"");
    }
    do
    {
        ret = NB_Send_Cmd(buf, "OK", 20);
    } while (ret == AT_ERROR);

    return (ret);
}

// 发布消息
int nb_mqtt_pub(char *topic, char *buf)
{
    int ret;
    int msgLen = strlen(buf);
    char wbuf[1024];
    hex_to_hexStr(buf, msgLen, tmpbuf);
    snprintf(wbuf, sizeof(wbuf), "AT+MQTTPUB=\"%s\",2,1,0,%d,\"%s\"", topic, msgLen, tmpbuf);
    do
    {
        ret=NB_Send_Cmd(wbuf, "OK", 20);
    } while (ret == AT_ERROR);

    return (ret);
}

// 订阅主题
int nb_mqtt_sub(char *topic)
{
    int ret;
    char wbuf[1024];
    snprintf(wbuf, sizeof(wbuf), "AT+MQTTSUB=\"%s\",0", topic);
    ret = NB_Send_Cmd(wbuf, "OK", 20);
    if (ret != AT_OK)
    {
        printf("Message sub fail!\n");
        return (ret);
    }
    return (AT_OK);
}