#include "bsp_robotic_arm.h"

#include "at32f403a_407_clock.h"
#include "at32f403a_407_gpio.h"

static uint8_t robotic_arm_set_angle(RoboticArm_NO no, int8_t angle, uint8_t speed);
static void robotic_arm_enable(void);
static void robotic_arm_disable(void);

/************************ describe the rotation of the steering gear (full speed) ***********************************/

typedef struct {                // stepping:10ua
    uint16_t end_val;
    uint16_t cur_val;
    uint16_t hop_val;
    uint16_t set_hop_val;        // used to ensure completion of previous cycle execution
    gpio_type *gpio;
    uint32_t pin;
}Engine_Param_t;

Engine_Param_t engines[6] = {
    {2000, 0, 0, 0, GPIOB, GPIO_PINS_10},
    {2000, 0, 0, 0, GPIOB, GPIO_PINS_11},
    {2000, 0, 0, 0, GPIOB, GPIO_PINS_13},
    {2000, 0, 0, 0, GPIOD, GPIO_PINS_12},
    {2000, 0, 0, 0, GPIOD, GPIO_PINS_13},
    {2000, 0, 0, 0, GPIOC, GPIO_PINS_8},
};

__inline static void engine_param_reset(void)
{
    for (int i=0; i<6; ++i) {
        Engine_Param_t *e = &engines[i];
        e->cur_val = 0;
        e->gpio->clr = e->pin;
    }
}

__inline static void engine_dispatch_10us(void)
{
    for (int i=0; i<6; ++i) {
        Engine_Param_t *e = &engines[i];
        if (e->cur_val == 0) {
            if (e->hop_val == 0) e->gpio->clr = e->pin;
            else                 e->gpio->scr = e->pin;
        } else if (e->cur_val == e->hop_val) {
            e->gpio->clr =  e->pin;
        } else if (e->cur_val == e->end_val) {
            e->gpio->scr =  e->pin;
            e->cur_val = 0;
            if (e->set_hop_val) {
                e->hop_val = e->set_hop_val;
                e->set_hop_val = 0;
            }
        }
        ++e->cur_val;
    }
}

__inline static void engine_set_angle(RoboticArm_NO no, int8_t angle)
{
    if (angle > 90)  angle = 90;
    if (angle < -90) angle = -90;

    engines[no].set_hop_val = 200.0f / 180.0f * (angle + 90) + 50;
}

/************************ describe the rotation of the steering gear (adjustable speed) ****************************/
typedef struct {
    int8_t end_val;
    int8_t cur_val;
    uint16_t end_step_val;
    uint16_t cur_step_val;
}EngineSpeed_Param_t;

EngineSpeed_Param_t engine_speeds[6] = {
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
    {0, 0, 0, 0},
};

__inline static void engine_speed_dispatch_3000us(void)
{
    for (int i=0; i<6; ++i) {
        EngineSpeed_Param_t *e = &engine_speeds[i];

        int16_t dif = e->end_val - e->cur_val;
        if (dif == 0) {
            continue;
        }

        if (e->end_step_val == 0) {
            e->cur_val = e->end_val;
            engine_set_angle(i, e->cur_val);
            continue;
        }

        dif = (dif > 0) ? 1 : -1;
        if (e->cur_step_val == e->end_step_val) {
            e->cur_step_val = 0;
            e->cur_val += dif;
            engine_set_angle(i, e->cur_val);
        }
        e->cur_step_val++;
    }
}

/************************ timer scheduling(10us) ****************************/

void TMR1_OVF_TMR10_IRQHandler(void)
{
    static int cnt = 0;

    if(tmr_flag_get(TMR10, TMR_OVF_FLAG) == SET) {

        engine_dispatch_10us();

        if (cnt == 300) {
            engine_speed_dispatch_3000us();
            cnt = 0;
        }

        cnt++;

        tmr_flag_clear(TMR10, TMR_OVF_FLAG);
    }
}

static void robotic_arm_tmr10_ovf_int_init_10us(void)
{
    crm_periph_clock_enable(CRM_TMR10_PERIPH_CLOCK, AT_TRUE);

    tmr_base_init(TMR10, 20 - 1, 120 - 1);

    tmr_cnt_dir_set(TMR10, TMR_COUNT_UP);

    nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);

    nvic_irq_enable(TMR1_OVF_TMR10_IRQn, 0, 0);

    tmr_interrupt_enable(TMR10, TMR_OVF_INT, AT_TRUE);

    tmr_counter_enable(TMR10, AT_TRUE);
}

/************************ software simulated gpio ****************************/

static void robotic_arm_gpio_init(void)
{
    gpio_init_type gpio_init_struct = {0};

    crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, AT_TRUE);
    crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, AT_TRUE);
    crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, AT_TRUE);

    gpio_default_para_init(&gpio_init_struct);

    gpio_init_struct.gpio_drive_strength    = GPIO_DRIVE_STRENGTH_STRONGER;
    gpio_init_struct.gpio_out_type          = GPIO_OUTPUT_PUSH_PULL;
    gpio_init_struct.gpio_mode              = GPIO_MODE_OUTPUT;
    gpio_init_struct.gpio_pull              = GPIO_PULL_NONE;

    gpio_init_struct.gpio_pins              = GPIO_PINS_10 | GPIO_PINS_11 | GPIO_PINS_13;
    gpio_init(GPIOB, &gpio_init_struct);

    gpio_init_struct.gpio_pins              = GPIO_PINS_12 | GPIO_PINS_13;
    gpio_init(GPIOD, &gpio_init_struct);

    gpio_init_struct.gpio_pins              = GPIO_PINS_8;
    gpio_init(GPIOC, &gpio_init_struct);
}

/***************************************************************************/

RoboticArm_Handler robotic_arm_handler = {
    .enable    = robotic_arm_enable,
    .disable   = robotic_arm_disable,
    .set_angle = robotic_arm_set_angle,
};

RoboticArm_Handler *robotic_arm_init(void)
{
    robotic_arm_gpio_init();

    robotic_arm_tmr10_ovf_int_init_10us();

    return &robotic_arm_handler;
}

void robotic_arm_uninit(void)
{
    robotic_arm_disable();
}

static uint8_t robotic_arm_set_angle(RoboticArm_NO no, int8_t angle, uint8_t speed)
{
    if (no >= RoboticArm_MAX) {
        return 1;
    }

    if (speed > 10) {
        speed = 10;
    }

    engine_speeds[no].end_val         = angle;
    engine_speeds[no].end_step_val    = (10 - speed);
    engine_speeds[no].cur_step_val    = engine_speeds[no].end_step_val;

    return 0;
}

static void robotic_arm_enable(void)
{
    engine_param_reset();

    for (int i=0; i<6; ++i) {
        EngineSpeed_Param_t *e = &engine_speeds[i];
        e->cur_val = e->end_val;
        engine_set_angle(i, e->end_val);
    }

    tmr_counter_enable(TMR6, AT_TRUE);
}


static void robotic_arm_disable(void)
{
    tmr_counter_enable(TMR6, AT_FALSE);

    engine_param_reset();
}