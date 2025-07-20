#include "bsp_encoder.h"
#include "at32f403a_407_clock.h"
#include "at32f403a_407_gpio.h"
#include "at32f403a_407_board.h"

#define ENCODER_TEST 0 // 1开启测试模式


static void encoder_exint_init(void);
static void encoder_input_gpio_init(void);

//  左前   左后   右前  右后
//   M1     M2    M2    M4
int g_Encoder_M1_Now = 0;
int g_Encoder_M2_Now = 0;
int g_Encoder_M3_Now = 0;
int g_Encoder_M4_Now = 0;

volatile Encoder_Cnt_t encoder_cnt_t = {0};

void encoder_init(void)
{
    encoder_input_gpio_init();
    encoder_exint_init();
}

void encoder_uninit(void)
{
    nvic_irq_disable(EXINT0_IRQn);
    nvic_irq_disable(EXINT1_IRQn);
    nvic_irq_disable(EXINT2_IRQn);
    nvic_irq_disable(EXINT3_IRQn);
    nvic_irq_disable(EXINT9_5_IRQn);
    nvic_irq_disable(EXINT15_10_IRQn);
}

static void encoder_exint_init(void)
{
    exint_init_type exint_init_struct;

    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

    // M4
    gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOB, GPIO_PINS_SOURCE0);
    gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOB, GPIO_PINS_SOURCE1);
    // M3
    gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOA, GPIO_PINS_SOURCE2);
    gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOA, GPIO_PINS_SOURCE3);
    // M2
    gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOB, GPIO_PINS_SOURCE9);
    gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOB, GPIO_PINS_SOURCE8);
    // M1
    gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOB, GPIO_PINS_SOURCE14);
    gpio_exint_line_config(GPIO_PORT_SOURCE_GPIOB, GPIO_PINS_SOURCE15);

    exint_default_para_init(&exint_init_struct);
    exint_init_struct.line_enable = TRUE;
    exint_init_struct.line_mode = EXINT_LINE_INTERRUPUT;
    exint_init_struct.line_select = EXINT_LINE_0 | EXINT_LINE_1 | EXINT_LINE_2 | EXINT_LINE_3 |
                                    EXINT_LINE_9 | EXINT_LINE_8 | EXINT_LINE_14 | EXINT_LINE_15;
    exint_init_struct.line_polarity = EXINT_TRIGGER_BOTH_EDGE;
    exint_init(&exint_init_struct);

    // M4
    nvic_irq_enable(EXINT0_IRQn, 1, 0);
    nvic_irq_enable(EXINT1_IRQn, 1, 0);
    // M3
    nvic_irq_enable(EXINT2_IRQn, 1, 0);
    nvic_irq_enable(EXINT3_IRQn, 1, 0);
    // M2    EXIT 9&8
    nvic_irq_enable(EXINT9_5_IRQn, 1, 0);
    // M1   EXIT 14&15
    nvic_irq_enable(EXINT15_10_IRQn, 1, 0);
}

static void encoder_input_gpio_init(void)
{
    gpio_init_type gpio_init_struct;

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

    gpio_default_para_init(&gpio_init_struct);

    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

    gpio_init_struct.gpio_pins = GPIO_PINS_3 | GPIO_PINS_2;
    gpio_init(GPIOA, &gpio_init_struct);

    gpio_init_struct.gpio_pins = GPIO_PINS_0 | GPIO_PINS_1 | GPIO_PINS_8 | GPIO_PINS_9 | GPIO_PINS_14 | GPIO_PINS_15;
    gpio_init(GPIOB, &gpio_init_struct);
}

void EXINT0_IRQHandler(void)
{
#if RUNHE_CAR
    if (exint_flag_get(EXINT_LINE_0) != RESET)
    {
        exint_flag_clear(EXINT_LINE_0);

        if (gpio_input_data_bit_read(GPIOB, GPIO_PINS_0) == RESET)
        {
            encoder_cnt_t.l3_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_1) == RESET) ? 1 : -1;
        }
        else
        {
            encoder_cnt_t.l3_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_1) == SET) ? 1 : -1;
        }
    }
#endif
}
void EXINT1_IRQHandler(void)
{
    if (exint_flag_get(EXINT_LINE_1) != RESET)
    {
        exint_flag_clear(EXINT_LINE_1);

        if (gpio_input_data_bit_read(GPIOB, GPIO_PINS_1) == RESET)
        {
            encoder_cnt_t.l3_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_0) == SET) ? 1 : -1;
        }
        else
        {
            encoder_cnt_t.l3_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_0) == RESET) ? 1 : -1;
        }
    }
}

void EXINT2_IRQHandler(void)
{
#if RUNHE_CAR
    if (exint_flag_get(EXINT_LINE_2) != RESET)
    {
        exint_flag_clear(EXINT_LINE_2);

        if (gpio_input_data_bit_read(GPIOA, GPIO_PINS_2) == RESET)
        {
            encoder_cnt_t.l2_cnt += (gpio_input_data_bit_read(GPIOA, GPIO_PINS_3) == RESET) ? 1 : -1;
        }
        else
        {
            encoder_cnt_t.l2_cnt += (gpio_input_data_bit_read(GPIOA, GPIO_PINS_3) == SET) ? 1 : -1;
        }
    }
#endif
}
void EXINT3_IRQHandler(void)
{
    if (exint_flag_get(EXINT_LINE_3) != RESET)
    {

        exint_flag_clear(EXINT_LINE_3);
        if (gpio_input_data_bit_read(GPIOA, GPIO_PINS_3) == RESET)
        {
            encoder_cnt_t.l2_cnt += (gpio_input_data_bit_read(GPIOA, GPIO_PINS_2) == SET) ? 1 : -1;
        }
        else
        {
            encoder_cnt_t.l2_cnt += (gpio_input_data_bit_read(GPIOA, GPIO_PINS_2) == RESET) ? 1 : -1;
        }
    }
}

void EXINT9_5_IRQHandler(void)
{
#if RUNHE_CAR
    if (exint_flag_get(EXINT_LINE_9) != RESET)
    {

        exint_flag_clear(EXINT_LINE_9);
        if (gpio_input_data_bit_read(GPIOB, GPIO_PINS_9) == RESET)
        {
            encoder_cnt_t.l4_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_8) == RESET) ? 1 : -1;
        }
        else
        {
            encoder_cnt_t.l4_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_8) == SET) ? 1 : -1;
        }
    }
    if (exint_flag_get(EXINT_LINE_8) != RESET)
    {

        exint_flag_clear(EXINT_LINE_8);
        if (gpio_input_data_bit_read(GPIOB, GPIO_PINS_8) == RESET)
        {
            encoder_cnt_t.l4_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_9) == SET) ? 1 : -1;
        }
        else
        {
            encoder_cnt_t.l4_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_9) == RESET) ? 1 : -1;
        }
    }
#endif
}

void EXINT15_10_IRQHandler(void)
{
#if RUNHE_CAR
    if (exint_flag_get(EXINT_LINE_14) != RESET)
    {

        exint_flag_clear(EXINT_LINE_14);
        if (gpio_input_data_bit_read(GPIOB, GPIO_PINS_14) == RESET)
        {
            encoder_cnt_t.l1_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_15) == RESET) ? 1 : -1;
        }
        else
        {
            encoder_cnt_t.l1_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_15) == SET) ? 1 : -1;
        }
    }
    if (exint_flag_get(EXINT_LINE_15) != RESET)
    {

        exint_flag_clear(EXINT_LINE_15);
        if (gpio_input_data_bit_read(GPIOB, GPIO_PINS_15) == RESET)
        {
            encoder_cnt_t.l1_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_14) == SET) ? 1 : -1;
        }
        else
        {
            encoder_cnt_t.l1_cnt += (gpio_input_data_bit_read(GPIOB, GPIO_PINS_14) == RESET) ? 1 : -1;
        }
    }
#endif
}

void Encoder_Update_Count(void)
{
#if !ENCODER_TEST
    // M1
    g_Encoder_M1_Now -= encoder_cnt_t.l1_cnt;
    encoder_cnt_t.l1_cnt = 0;
    // M2
    g_Encoder_M2_Now -= encoder_cnt_t.l4_cnt;
    encoder_cnt_t.l4_cnt = 0;
    // M3
    g_Encoder_M3_Now += encoder_cnt_t.l2_cnt;
    encoder_cnt_t.l2_cnt = 0;
    // M4
    g_Encoder_M4_Now += encoder_cnt_t.l3_cnt;
    encoder_cnt_t.l3_cnt = 0;
#else
    // M1
    g_Encoder_M1_Now = -encoder_cnt_t.l1_cnt;
    encoder_cnt_t.l1_cnt = 0;
    // M2
    g_Encoder_M2_Now = -encoder_cnt_t.l4_cnt;
    encoder_cnt_t.l4_cnt = 0;
    // M3
    g_Encoder_M3_Now = encoder_cnt_t.l2_cnt;
    encoder_cnt_t.l2_cnt = 0;
    // M4
    g_Encoder_M4_Now = encoder_cnt_t.l3_cnt;
    encoder_cnt_t.l3_cnt = 0;
#endif
}

void Encoder_Get_ALL(int *Encoder_all)
{
    Encoder_all[0] = g_Encoder_M1_Now;
    Encoder_all[1] = g_Encoder_M2_Now;
    Encoder_all[2] = g_Encoder_M3_Now;
    Encoder_all[3] = g_Encoder_M4_Now;
}

// 发送当前的编码器数据到主控上
int Send_Encoder_All_Now[MAX_MOTOR] = {0};
void Send_Encoder_Data(void)
{

#define ENCODER_LEN 21

    Encoder_Get_ALL(Send_Encoder_All_Now);
    uint8_t data_buffer[ENCODER_LEN] = {0};
    uint8_t i, checknum = 0;
    data_buffer[0] = 0xFF;
    data_buffer[1] = 0XFB;
    data_buffer[2] = ENCODER_LEN - 2; // 数量
    data_buffer[3] = 0x0D;            // 功能位
    // Encoder_M1
    data_buffer[4] = Send_Encoder_All_Now[0] & 0xFF;
    data_buffer[5] = (Send_Encoder_All_Now[0] >> 8) & 0xFF;
    data_buffer[6] = (Send_Encoder_All_Now[0] >> 16) & 0xFF;
    data_buffer[7] = (Send_Encoder_All_Now[0] >> 24) & 0xFF;
    // Encoder_M2
    data_buffer[8] = Send_Encoder_All_Now[1] & 0xFF;
    data_buffer[9] = (Send_Encoder_All_Now[1] >> 8) & 0xFF;
    data_buffer[10] = (Send_Encoder_All_Now[1] >> 16) & 0xFF;
    data_buffer[11] = (Send_Encoder_All_Now[1] >> 24) & 0xFF;
    // Encoder_M3
    data_buffer[12] = Send_Encoder_All_Now[2] & 0xFF;
    data_buffer[13] = (Send_Encoder_All_Now[2] >> 8) & 0xFF;
    data_buffer[14] = (Send_Encoder_All_Now[2] >> 16) & 0xFF;
    data_buffer[15] = (Send_Encoder_All_Now[2] >> 24) & 0xFF;
    // Encoder_M4
    data_buffer[16] = Send_Encoder_All_Now[3] & 0xFF;
    data_buffer[17] = (Send_Encoder_All_Now[3] >> 8) & 0xFF;
    data_buffer[18] = (Send_Encoder_All_Now[3] >> 16) & 0xFF;
    data_buffer[19] = (Send_Encoder_All_Now[3] >> 24) & 0xFF;

    for (i = 2; i < ENCODER_LEN - 1; i++)
    {
        checknum += data_buffer[i];
    }
    data_buffer[ENCODER_LEN - 1] = checknum;
    usartdmasend(data_buffer, sizeof(data_buffer));
}
