#include "bsp_tracing.h"
int Map(int value, int fromLow, int fromHigh, int toLow, int toHigh)
{
  return (value - fromLow) * (toHigh - toLow) / (fromHigh - fromLow) + toLow;
}

/*******************************************************************************/
int input_value0;
int input_value1;
int input_value2;
int input_value3;
int input_value4;

int Motor1_Speed = 0;
int Motor2_Speed = 0;
int Motor3_Speed = 0;
int Motor4_Speed = 0;

/*******************************************************************************/

static void GPIO_init(void)
{
  // gpio结构体
  gpio_init_type gpio_init_struct;

  // 开启GPIOD的时钟,CRM_GPIOD_PERIPH_CLOCK
  crm_periph_clock_enable(CRM_GPIOD_PERIPH_CLOCK, TRUE);

  /* set default parameter */
  gpio_default_para_init(&gpio_init_struct);

  // io管脚
  gpio_init_struct.gpio_pins = GPIO_PINS_0 | GPIO_PINS_1 | GPIO_PINS_2 | GPIO_PINS_3 | GPIO_PINS_4;

  // 输入模式
  gpio_init_struct.gpio_mode = GPIO_MODE_INPUT;

  // 无上下拉电阻
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

  /*
  GPIO_DRIVE_STRENGTH_MODERATE 对应适中的电流推动/吸入能力
  GPIO_DRIVE_STRENGTH_STRONGER 对应较大的电流推动/吸入能力
  GPIO_DRIVE_STRENGTH_MAXIMUM 对应极大的电流推动/吸入能力
  如果 IO 速度设置为最大的推动力设置，且负载较小时，易在 IO 上产生过冲振铃现象，存在影响应用的可能性
  */
  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;

  // 初始化gpio
  gpio_init(GPIOD, &gpio_init_struct);
}
// 循迹初始化
void Trace_init()
{
  GPIO_init();
}
/*******************************************************************************/
// PID 循迹
int timer_periodic(void)
{
    input_value0 = gpio_input_data_bit_read(GPIOD, GPIO_PINS_0); // 右边
    input_value1 = gpio_input_data_bit_read(GPIOD, GPIO_PINS_1); // 右边
    input_value2 = gpio_input_data_bit_read(GPIOD, GPIO_PINS_2); // 中间
    input_value3 = gpio_input_data_bit_read(GPIOD, GPIO_PINS_3); // 左边
    input_value4 = gpio_input_data_bit_read(GPIOD, GPIO_PINS_4); // 左边


    // Motor_Set_Pwm(0,1000);
    // Motor_Set_Pwm(1,1000);
    // Motor_Set_Pwm(2,1000);
    // Motor_Set_Pwm(3,1000);

    if (input_value0 == 1 && input_value4 == 1) // 不转
    {

      // 控制四个电机的转速
      Mecanum_State(0, 0, 0);

      printf("stop\n");
    }

    if (input_value0 == 0 && input_value4 == 0) // 两端的红外识别白色，直行
    {

      Mecanum_State(1, 200, 0);
      // Mecanum_State(1,350);
      // printf("forward\n");
    }

    if (input_value0 == 1 && input_value1 == 1 && input_value4 == 0) // 左边的识别到白色，右边识别到黑色。 说明小车左偏，需要右转，
    {

      Mecanum_State(6, 200, 0);
      // Mecanum_State(6,350);
      // printf("right\n");
    }
    if (input_value0 == 1 && input_value4 == 0) // 左边的识别到白色，右边识别到黑色。 说明小车左偏，需要右转，
    {
      Mecanum_State(6, 200, 0);
      // Mecanum_State(6,350);
      // printf("right\n");
    }

    if (input_value0 == 0 && input_value3 == 1 && input_value4 == 1) // 左边识别到黑色，右边识别  说明小车右偏，需要左转，
    {
      Mecanum_State(5, 200, 0);
      // Mecanum_State(5, 350);
      // printf("left\n");
    }
    if (input_value0 == 0 && input_value4 == 1) // 左边识别到黑色，右边识别  说明小车右偏，需要左转，
    {
      Mecanum_State(5, 200, 0);
      // Mecanum_State(5, 350);
      // printf("left\n");
    }
    osDelay(3);
}

// 开启循迹：1    关闭循迹：0
void Trace_Ctrl(uint8_t ctrl_flag)
{
  // 开启循迹
  if (ctrl_flag == 1)
  {
    osThreadResume(trcae_tid);
  }
  else if (ctrl_flag == 0)
  {
    osThreadSuspend(trcae_tid);
  }
}
