#include "bsp_timer14.h"

void bsp_timer14_init(void)
{
    crm_periph_clock_enable(CRM_TMR14_PERIPH_CLOCK, TRUE);
    tmr_base_init(TMR14, 1000 - 1, 240 - 1);//240 000 000 /  1000 /240=1000hz=1ms
    tmr_cnt_dir_set(TMR14, TMR_COUNT_UP);
    tmr_clock_source_div_set(TMR14, TMR_CLOCK_DIV1);
    tmr_period_buffer_enable(TMR14, FALSE);
    nvic_irq_enable(TMR8_TRG_HALL_TMR14_IRQn, 3,1);
    tmr_interrupt_enable(TMR14, TMR_OVF_INT, TRUE);
    tmr_counter_enable(TMR14, TRUE);
}
uint16_t tmr14_cnt = 0;
void TMR8_TRG_HALL_TMR14_IRQHandler(void)
{
    if (tmr_flag_get(TMR14, TMR_OVF_FLAG) != RESET)
    {
        tmr14_cnt++;
        if (tmr14_cnt >= 10)
        {
            tmr14_cnt = 0;
            MPU9250_Read_Data_Handle();
            // printf("tmr14_cnt:%d\n", tmr14_cnt);
            // IMU();
            // printf("imu.yaw:%f,%f,%f\r\n", imu.yaw, imu.pitch, imu.roll);
        }
        tmr_flag_clear(TMR14, TMR_OVF_FLAG); // 清除标志位
    }
}