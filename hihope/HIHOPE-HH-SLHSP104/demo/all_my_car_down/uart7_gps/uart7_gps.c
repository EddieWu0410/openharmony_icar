#include "uart7_gps.h"
_SaveData Save_Data;
void uart7_init(uint32_t bound)
{
	gpio_init_type gpio_init_struct;
	/*Enable the UART7 Clock*/
	crm_periph_clock_enable(CRM_GPIOE_PERIPH_CLOCK, TRUE); // 开启GPIOE的时钟
	crm_periph_clock_enable(CRM_UART7_PERIPH_CLOCK, TRUE); // 开启UART7的时钟

	gpio_default_para_init(&gpio_init_struct);
	/* Configure the UART7 TX pin */
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER; // 较大电流推动/吸入能力
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;				 // 推挽输出
	gpio_init_struct.gpio_mode = GPIO_MODE_MUX;							 // 复用
	gpio_init_struct.gpio_pins = GPIO_PINS_8;							 // PE8
	gpio_init_struct.gpio_pull = GPIO_PULL_NONE;						 // 无上下拉
	gpio_init(GPIOE, &gpio_init_struct);

	/* Configure the UART7 RX pin */
	gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER; // 较大电流推动/吸入能力
	gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;				 // 推挽输出
	gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;						 // 输入模式
	gpio_init_struct.gpio_pins = GPIO_PINS_7;							 // PE7
	gpio_init_struct.gpio_pull = GPIO_PULL_UP;							 // 上拉
	gpio_init(GPIOE, &gpio_init_struct);

	nvic_irq_enable(UART7_IRQn, 2, 1); // 使能串口7中断，优先级0，次优先级6

	/*Configure UART param*/
	usart_init(UART7, bound, USART_DATA_8BITS, USART_STOP_1_BIT);	  // 波特率，8数据位，1停止位
	usart_hardware_flow_control_set(UART7, USART_HARDWARE_FLOW_NONE); // 无硬件流操作
	usart_parity_selection_config(UART7, USART_PARITY_NONE);		  // 无校验
	usart_transmitter_enable(UART7, TRUE);							  // 使能发送
	usart_receiver_enable(UART7, TRUE);								  // 使能接收

	usart_interrupt_enable(UART7, USART_RDBF_INT, TRUE); // 使能串口接收中断
	usart_interrupt_enable(UART7, USART_IDLE_INT, TRUE); // 使能串口空闲中断
	usart_enable(UART7, TRUE);							 // 使能串口
}


//串口7的中断函数
#define GPSRX_LEN_MAX 255
unsigned char GPSRX_BUFF[GPSRX_LEN_MAX];
unsigned char GPSRX_LEN = 0;

void UART7_IRQHandler(void)
{
	uint8_t Res;
	if (usart_flag_get(UART7, USART_RDBF_FLAG) != RESET)
	{
		usart_flag_clear(UART7, USART_RDBF_FLAG);
		// 获取寄存器里的数据
		Res = usart_data_receive(UART7);
		// 缓存到buffer中
		// g_rx_buffer[g_rx_cnt++] = data;
		// 避免缓冲区溢出 (可选)

		if (Res == '$')
			GPSRX_LEN = 0;
		GPSRX_BUFF[GPSRX_LEN++] = Res;
		// if (g_rx_cnt >= RX_BUFFER_LEN)
		// 	g_rx_cnt = 0;
		if (GPSRX_BUFF[0] == '$' && GPSRX_BUFF[4] == 'M' && GPSRX_BUFF[5] == 'C') // 确定是否收到"GPRMC/GNRMC"这一帧数据
		{
			if (Res == '\n')
			{
				memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);	 // 清空
				memcpy(Save_Data.GPS_Buffer, GPSRX_BUFF, GPSRX_LEN); // 保存数据
				Save_Data.isGetData = 1;
				GPSRX_LEN = 0;
				memset(GPSRX_BUFF, 0, GPSRX_LEN_MAX); // 清空
			}
		}
		if (GPSRX_LEN >= GPSRX_LEN_MAX)
		{
			GPSRX_LEN = GPSRX_LEN_MAX;
		}
#if (recv_view == 1)
		usart_data_transmit(UART7, data);
#endif
	}
}


//在GPS数据中，识别是否有想要的串口命令
uint8_t Hand(char *a)                   
{ 
    if(strstr((const char*)GPSRX_BUFF,a)!=NULL)
            return 1;
        else
                return 0;
}
//清除串口接收的数据
void CLR_Buf(void)                           
{
    memset(GPSRX_BUFF, 0, GPSRX_LEN_MAX);      //清空
    GPSRX_LEN = 0;                    
}
//清除GPS结构体数据
void clrStruct(void)
{
        Save_Data.isGetData = 0;
        Save_Data.isParseData = 0;
        Save_Data.isUsefull = 0;
        memset(Save_Data.GPS_Buffer, 0, GPS_Buffer_Length);      //清空
        memset(Save_Data.UTCTime,    0, UTCTime_Length);
        memset(Save_Data.latitude,   0, latitude_Length);
        memset(Save_Data.N_S,        0, N_S_Length);
        memset(Save_Data.longitude,  0, longitude_Length);
        memset(Save_Data.E_W,        0, E_W_Length);
        
}
//错误日志打印
void errorLog(int num)
{
	while (1)
	{
		printf("GPS_ERROR:%d\r\n", num);
	}
}


//解析GPS发送过来的数据
void parseGpsBuffer(void)
{
	char *subString;
	char *subStringNext;
	char i = 0;
	if (Save_Data.isGetData)
	{
		Save_Data.isGetData = 0;
		printf("**************\r\n");
		printf("%s\r\n", Save_Data.GPS_Buffer);

		for (i = 0; i <= 6; i++)
		{
			if (i == 0)
			{
				if ((subString = strstr(Save_Data.GPS_Buffer, ",")) == NULL)
					errorLog(1); // 解析错误
			}
			else
			{
				subString++;
				if ((subStringNext = strstr(subString, ",")) != NULL)
				{
					char usefullBuffer[2];
					switch (i)
					{
					case 1:
						memcpy(Save_Data.UTCTime, subString, subStringNext - subString);
						break; // 获取UTC时间
					case 2:
						memcpy(usefullBuffer, subString, subStringNext - subString);
						break; // 获取UTC时间
					case 3:
						memcpy(Save_Data.latitude, subString, subStringNext - subString);
						break; // 获取纬度信息
					case 4:
						memcpy(Save_Data.N_S, subString, subStringNext - subString);
						break; // 获取N/S
					case 5:
						memcpy(Save_Data.longitude, subString, subStringNext - subString);
						break; // 获取经度信息
					case 6:
						memcpy(Save_Data.E_W, subString, subStringNext - subString);
						break; // 获取E/W

					default:
						break;
					}
					subString = subStringNext;
					Save_Data.isParseData = 1;
					if (usefullBuffer[0] == 'A')
						Save_Data.isUsefull = 1;
					else if (usefullBuffer[0] == 'V')
						Save_Data.isUsefull = 0;
				}
				else
				{
					errorLog(2); // 解析错误
				}
			}
		}
	}
}
//输出解析后的数据
void printGpsBuffer(void)
{
	// if (Save_Data.isParseData)
    if(1)
	{
		Save_Data.isParseData = 0;

		printf("Save_Data.UTCTime = ");
		printf("%s", Save_Data.UTCTime);
		printf("\r\n");

		if (Save_Data.isUsefull)
		{
			Save_Data.isUsefull = 0;
			// 串口显示纬度
			printf("Save_Data.latitude = ");
			printf("%s", Save_Data.latitude);

			// 串口显示
			printf("Save_Data.N_S = ");
			printf("%s\r\n", Save_Data.N_S);

			// 串口显示经度
			printf("Save_Data.longitude = ");
			printf("%s", Save_Data.longitude);
			printf("\r\n");

			// 串口显示
			printf("Save_Data.E_W = ");
			printf("%s", Save_Data.E_W);
			printf("\r\n");
		}
		else
		{
			printf("GPS DATA is not usefull!\r\n");
		}
	}
}
