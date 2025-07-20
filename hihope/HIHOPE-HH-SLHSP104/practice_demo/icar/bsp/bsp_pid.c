#include "bsp_pid.h"

#include "at32f403a_407_clock.h"

typedef struct {
    int16_t target_val;
    int16_t last_error;
    int16_t integral;
    int16_t cnt;
}Ctrl_Parameter_t;

static void pid_dispatch(void);
static void encoder_timer_init_50ms(void);
static void pid_set_target_val(int index, int val);

static set_val_func g_set_val = 0;
static get_val_func g_get_val = 0;

static volatile Ctrl_Parameter_t ctrl_parameter[PID_CNT] = {0};

static PID_Cal_t g_pid = {
    .set_target_val = pid_set_target_val
};

PID_Cal_t *pid_init(set_val_func set_val, get_val_func get_val)
{
    g_set_val = set_val;
    g_get_val = get_val;

    encoder_timer_init_50ms();

    return &g_pid;
}

void pid_uninit(void)
{
    nvic_irq_disable(TMR7_GLOBAL_IRQn);
}

static void encoder_timer_init_50ms(void)
{
    crm_periph_clock_enable(CRM_TMR7_PERIPH_CLOCK, AT_TRUE);
    //120 000 000 /24000  50 000 500hz
    tmr_base_init(TMR7, 100 - 1, 24000 - 1);

    tmr_cnt_dir_set(TMR7, TMR_COUNT_UP);

    tmr_interrupt_enable(TMR7, TMR_OVF_INT, AT_TRUE);
    
    nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
    nvic_irq_enable(TMR7_GLOBAL_IRQn, 2, 0); 
    tmr_counter_enable(TMR7, AT_TRUE);
}

void TMR7_GLOBAL_IRQHandler(void)
{
    if(tmr_flag_get(TMR7, TMR_OVF_FLAG) == SET) {
        tmr_flag_clear(TMR7, TMR_OVF_FLAG);

        pid_dispatch();
    }
}

static void pid_dispatch(void)
{
    if (g_set_val == 0 || g_get_val == 0) {
        return;
    }

    for (uint8_t i=0; i<PID_CNT; ++i) {
        int16_t current_val = g_get_val(i, ctrl_parameter[i].cnt);
        int16_t error = ctrl_parameter[i].target_val - current_val;
        int16_t output = 0;
        int16_t derivative = 0;

        ctrl_parameter[i].integral += error;

        if (abs(error) < 3 && current_val == 0) {
             ctrl_parameter[i].integral = 0;
        }

        if (ctrl_parameter[i].integral > 1000) {
            ctrl_parameter[i].integral = 1000;
        }
        else if (ctrl_parameter[i].integral < -1000) {
            ctrl_parameter[i].integral = -1000;
        }

        derivative = error - ctrl_parameter[i].last_error;
        ctrl_parameter[i].last_error = error;
        output = bsp_Kp * error + bsp_Ki * ctrl_parameter[i].integral + bsp_Kd * derivative;

        if (ctrl_parameter[i].target_val != 0) {
            ctrl_parameter[i].cnt = 100;
        }
        if (ctrl_parameter[i].target_val == 0 && ctrl_parameter[i].cnt != 0) {
            ctrl_parameter[i].cnt--;
            ctrl_parameter[i].integral = 0;
            ctrl_parameter[i].last_error = 0;
            output = 0;
        }

        g_set_val(i, output);
    }                
}

static void pid_set_target_val(int index, int val)
{
    if (index >= PID_CNT) {
        return;
    }

    ctrl_parameter[index].target_val = val;
}

