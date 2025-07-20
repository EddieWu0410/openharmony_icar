#include "bsp_motor.h"

#include "at32f403a_407_clock.h"
#include "at32f403a_407_board.h"
#include "app_motion.h"
#include "bsp_encoder.h"

static void motor_crm_configuration(void);
static void motor_gpio_configuration(void);
static void motor_tmr_configuration(void);

#define YABO 1
// #define RUNHE   2

static int16_t Motor_Ignore_Dead_Zone(int16_t pulse)
{

    if (pulse > 0)
        return pulse + MOTOR_IGNORE_PULSE;
    if (pulse < 0)
        return pulse - MOTOR_IGNORE_PULSE;
    return 0;
}

void motor_init(void)
{
    motor_crm_configuration();

    motor_gpio_configuration();

    motor_tmr_configuration();
}

static void motor_crm_configuration(void)
{
    crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);

    crm_periph_clock_enable(CRM_TMR1_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_TMR2_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_TMR4_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_TMR9_PERIPH_CLOCK, TRUE);

    crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);
    crm_periph_clock_enable(CRM_GPIOE_PERIPH_CLOCK, TRUE);
}

static void motor_gpio_configuration(void)
{
    gpio_init_type gpio_init_struct = {0};

    gpio_init_struct.gpio_pins = GPIO_PINS_0 | GPIO_PINS_1;
    gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
    gpio_init_struct.gpio_out_type = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_pull = GPIO_PULL_NONE;
    gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init(GPIOA, &gpio_init_struct);

    gpio_init_struct.gpio_pins = GPIO_PINS_14 | GPIO_PINS_15;
    gpio_init(GPIOD, &gpio_init_struct);

    gpio_init_struct.gpio_pins = GPIO_PINS_5 | GPIO_PINS_6 | GPIO_PINS_9 | GPIO_PINS_10;
    gpio_init(GPIOE, &gpio_init_struct);

    gpio_pin_remap_config(TMR1_GMUX_0011, TRUE);
    gpio_pin_remap_config(TMR4_MUX, TRUE);
    gpio_pin_remap_config(TMR9_MUX, TRUE);
}

static void motor_tmr_configuration(void)
{
    tmr_output_config_type tmr_output_struct = {0};
    // 240 000 000Hz/4000/3 =20khz

    tmr_base_init(TMR1, 4000 - 1, 3 - 1);
    tmr_base_init(TMR2, 4000 - 1, 3 - 1);
    tmr_base_init(TMR4, 4000 - 1, 3 - 1);
    tmr_base_init(TMR9, 4000 - 1, 3 - 1);

    // 240M
    tmr_clock_source_div_set(TMR1, TMR_CLOCK_DIV1);
    tmr_clock_source_div_set(TMR2, TMR_CLOCK_DIV1);
    tmr_clock_source_div_set(TMR4, TMR_CLOCK_DIV1);
    tmr_clock_source_div_set(TMR9, TMR_CLOCK_DIV1);

    tmr_cnt_dir_set(TMR1, TMR_COUNT_UP);
    tmr_cnt_dir_set(TMR2, TMR_COUNT_UP);
    tmr_cnt_dir_set(TMR4, TMR_COUNT_UP);
    tmr_cnt_dir_set(TMR9, TMR_COUNT_UP);

    tmr_clock_source_div_set(TMR1, TMR_CLOCK_DIV1); // 240M
    tmr_clock_source_div_set(TMR2, TMR_CLOCK_DIV1); // 240M
    tmr_clock_source_div_set(TMR4, TMR_CLOCK_DIV1); // 240M
    tmr_clock_source_div_set(TMR9, TMR_CLOCK_DIV1); // 240M

    tmr_output_default_para_init(&tmr_output_struct);
    tmr_output_struct.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
    tmr_output_struct.oc_output_state = TRUE;
    tmr_output_struct.oc_polarity = TMR_OUTPUT_ACTIVE_HIGH;

    tmr_output_channel_config(TMR1, TMR_SELECT_CHANNEL_1, &tmr_output_struct);
    tmr_output_channel_config(TMR2, TMR_SELECT_CHANNEL_1, &tmr_output_struct);
    tmr_output_channel_config(TMR2, TMR_SELECT_CHANNEL_2, &tmr_output_struct);
    tmr_output_channel_config(TMR4, TMR_SELECT_CHANNEL_4, &tmr_output_struct);
    tmr_output_channel_config(TMR4, TMR_SELECT_CHANNEL_3, &tmr_output_struct);
    tmr_output_channel_config(TMR9, TMR_SELECT_CHANNEL_1, &tmr_output_struct);
    tmr_output_channel_config(TMR9, TMR_SELECT_CHANNEL_2, &tmr_output_struct);

    tmr_output_default_para_init(&tmr_output_struct);
    tmr_output_struct.oc_mode = TMR_OUTPUT_CONTROL_PWM_MODE_A;
    tmr_output_struct.oc_output_state = FALSE;
    // tmr_output_struct.oc_polarity = TMR_OUTPUT_ACTIVE_LOW;
    tmr_output_struct.occ_output_state = TRUE;
    tmr_output_struct.occ_polarity = TMR_OUTPUT_ACTIVE_HIGH;

    tmr_output_channel_config(TMR1, TMR_SELECT_CHANNEL_2, &tmr_output_struct);

    tmr_output_enable(TMR1, TRUE); // 高级定时特有

    tmr_counter_enable(TMR1, TRUE);
    tmr_counter_enable(TMR2, TRUE);
    tmr_counter_enable(TMR4, TRUE);
    tmr_counter_enable(TMR9, TRUE);
}

void Motor_Stop(uint8_t brake)
{
    if (brake != 0)
        brake = 1;
    tmr_channel_value_set(TMR1, TMR_SELECT_CHANNEL_1, brake * MOTOR_MAX_PULSE);
    tmr_channel_value_set(TMR1, TMR_SELECT_CHANNEL_2, brake * MOTOR_MAX_PULSE);

    tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_1, brake * MOTOR_MAX_PULSE);
    tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_2, brake * MOTOR_MAX_PULSE);

    tmr_channel_value_set(TMR4, TMR_SELECT_CHANNEL_4, brake * MOTOR_MAX_PULSE);
    tmr_channel_value_set(TMR4, TMR_SELECT_CHANNEL_3, brake * MOTOR_MAX_PULSE);

    tmr_channel_value_set(TMR9, TMR_SELECT_CHANNEL_1, brake * MOTOR_MAX_PULSE);
    tmr_channel_value_set(TMR9, TMR_SELECT_CHANNEL_2, brake * MOTOR_MAX_PULSE);
}

void Motor_Set_Pwm(uint8_t id, int16_t speed)
{
// printf("Motor_Set_Pwm \n");
#if YABO_CAR
    speed = -speed;
#endif

    int16_t pulse = Motor_Ignore_Dead_Zone(speed);
    // printf("pulse : %d\n",pulse);

    if (pulse >= MOTOR_MAX_PULSE)
        pulse = MOTOR_MAX_PULSE;
    if (pulse <= -MOTOR_MAX_PULSE)
        pulse = -MOTOR_MAX_PULSE;

    switch (id)
    {
    case MOTOR_ID_M1:
    {

        if (pulse >= 0)
        {
            tmr_channel_value_set(TMR4, TMR_SELECT_CHANNEL_4, pulse);
            tmr_channel_value_set(TMR4, TMR_SELECT_CHANNEL_3, 0);
        }
        else
        {
            pulse = -pulse;
            tmr_channel_value_set(TMR4, TMR_SELECT_CHANNEL_4, 0);
            tmr_channel_value_set(TMR4, TMR_SELECT_CHANNEL_3, pulse);
        }
        break;
    }
    case MOTOR_ID_M2:
    {
        if (pulse >= 0)
        {
            tmr_channel_value_set(TMR9, TMR_SELECT_CHANNEL_1, pulse);
            tmr_channel_value_set(TMR9, TMR_SELECT_CHANNEL_2, 0);
        }
        else
        {
            pulse = -pulse;
            tmr_channel_value_set(TMR9, TMR_SELECT_CHANNEL_1, 0);
            tmr_channel_value_set(TMR9, TMR_SELECT_CHANNEL_2, pulse);
        }
        break;
    }
    case MOTOR_ID_M3:
    {

        if (pulse >= 0)
        {
            tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_1, 0);
            tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_2, pulse);
        }
        else
        {
            pulse = -pulse;
            tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_1, pulse);
            tmr_channel_value_set(TMR2, TMR_SELECT_CHANNEL_2, 0);
        }
        break;
    }

    case MOTOR_ID_M4:
    {
        if (pulse >= 0)
        {

            tmr_channel_value_set(TMR1, TMR_SELECT_CHANNEL_1, 0);
            tmr_channel_value_set(TMR1, TMR_SELECT_CHANNEL_2, pulse);
        }
        else
        {
            pulse = -pulse;
            tmr_channel_value_set(TMR1, TMR_SELECT_CHANNEL_1, pulse);
            tmr_channel_value_set(TMR1, TMR_SELECT_CHANNEL_2, 0);
        }
        break;
    }

    default:
        break;
    }
}
