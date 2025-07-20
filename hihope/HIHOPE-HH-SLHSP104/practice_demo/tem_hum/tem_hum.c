#include <stdio.h>
#include <unistd.h>
#include "cmsis_os2.h"
#include "ohos_init.h"
#include "tem_hum.h"

#define AHT20_STARTUP_TIME (20 * 1000)     // 上电启动时间
#define AHT20_CALIBRATION_TIME (40 * 1000) // 初始化（校准）时间
#define AHT20_MEASURE_TIME (75 * 1000)     // 测量时间

#define AHT20_CMD_CALIBRATION 0xBE // 初始化（校准）命令
#define AHT20_CMD_CALIBRATION_ARG0 0x08
#define AHT20_CMD_CALIBRATION_ARG1 0x00

int state = 0;
void i2c_lowlevel_init(i2c_handle_type *hi2c)
{
    gpio_init_type gpio_config;
    switch (state)
    {
    case 0:
        printf("i2c_lowlevel_init: I2Cx_PORT[I2C-1]  \n");

        /* i2c periph clock enable */
        crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
        crm_periph_clock_enable(CRM_I2C1_PERIPH_CLOCK, TRUE);
        crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK, TRUE);

        /* gpio configuration */
        gpio_config.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
        gpio_config.gpio_pull = GPIO_PULL_UP;
        gpio_config.gpio_mode = GPIO_MODE_MUX;
        gpio_config.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;

        gpio_config.gpio_pins = GPIO_PINS_6 | GPIO_PINS_7;
        gpio_init(GPIOB, &gpio_config);

        i2c_init(hi2c->i2cx, I2C_FSMODE_DUTY_2_1, 100000);
        i2c_own_address1_set(hi2c->i2cx, I2C_ADDRESS_MODE_7BIT, 0x00);
        // i2c_ack_enable(I2C1, TRUE);
        break;
    case 1:
        printf("i2c_lowlevel_init: I2Cx_PORT[I2C-3]  \n");
        /* i2c periph clock enable */
        crm_periph_clock_enable(CRM_IOMUX_PERIPH_CLOCK, TRUE);
        crm_periph_clock_enable(CRM_I2C3_PERIPH_CLOCK, TRUE);
        crm_periph_clock_enable(CRM_GPIOA_PERIPH_CLOCK, TRUE);
        crm_periph_clock_enable(CRM_GPIOC_PERIPH_CLOCK, TRUE);

        /* gpio configuration */
        gpio_config.gpio_out_type = GPIO_OUTPUT_OPEN_DRAIN;
        gpio_config.gpio_pull = GPIO_PULL_UP;
        gpio_config.gpio_mode = GPIO_MODE_MUX;
        gpio_config.gpio_drive_strength = GPIO_DRIVE_STRENGTH_MODERATE;

        /* configure i2c pins: scl */
        gpio_config.gpio_pins = GPIO_PINS_8;
        gpio_init(GPIOA, &gpio_config);

        /* configure i2c pins: sda */
        gpio_config.gpio_pins = GPIO_PINS_9;
        gpio_init(GPIOC, &gpio_config);

        i2c_init(hi2c->i2cx, I2C_FSMODE_DUTY_2_1, 100000);
        i2c_own_address1_set(hi2c->i2cx, I2C_ADDRESS_MODE_7BIT, 0x10);
        // i2c_ack_enable(I2C3, TRUE);
        break;

    default:
        break;
    }
}
void tem_hum_init()
{
    state = 0;
    hi2cx.i2cx = I2C1;
    i2c_config(&hi2cx);
}

/**
 * 传感器在采集时需要时间,主机发出测量指令（0xAC）后，延时75毫秒以上再读取转换后的数据并判断返回的状态位是否正常。
 * 若状态比特位[Bit7]为0代表数据可正常读取，为1时传感器为忙状态，主机需要等待数据处理完成。
 **/
#define AHT20_CMD_TRIGGER 0xAC // 触发测量命令
#define AHT20_CMD_TRIGGER_ARG0 0x33
#define AHT20_CMD_TRIGGER_ARG1 0x00

// 用于在无需关闭和再次打开电源的情况下，重新启动传感器系统，软复位所需时间不超过20 毫秒
#define AHT20_CMD_RESET 0xBA  // 软复位命令
#define AHT20_CMD_STATUS 0x71 // 获取状态命令

/**
 * STATUS 命令回复：
 * 1. 初始化后触发测量之前，STATUS 只回复 1B 状态值；
 * 2. 触发测量之后，STATUS 回复6B： 1B 状态值 + 2B 湿度 + 4b湿度 + 4b温度 + 2B 温度
 *      RH = Srh / 2^20 * 100%
 *      T  = St  / 2^20 * 200 - 50
 **/
#define AHT20_STATUS_BUSY_SHIFT 7 // bit[7] Busy indication
#define AHT20_STATUS_BUSY_MASK (0x1 << AHT20_STATUS_BUSY_SHIFT)

i2c_handle_type hi2cx;
uint32_t AHT20_Read(uint8_t *buffer, uint32_t buffLen)
{
    i2c_status_type i2c_status = I2C_ERR_INTERRUPT;
    /* start the request reception process */
    if ((i2c_status = i2c_master_receive(&hi2cx, AHT20_WRITE_ADDR, buffer, buffLen, 0xFFFF)) != I2C_OK)
    {
        printf("AHT20_Read: NG\n");
    }
    return i2c_status;
}

uint32_t AHT20_Write(uint8_t *buffer, uint32_t buffLen)
{
    i2c_status_type i2c_status = I2C_ERR_INTERRUPT;
    /* start the request reception process */
    if ((i2c_status = i2c_master_transmit(&hi2cx, AHT20_WRITE_ADDR, buffer, buffLen, 0xFFFF)) != I2C_OK)
    {
        printf("AHT20_Write: NG\n");
    }
    return i2c_status;
}
//-----------------------------------------------------------
// #define AHT20_STATUS_BUSY(status) (((status) & AHT20_STATUS_BUSY_MASK) >> AHT20_STATUS_BUSY_SHIFT)
uint8_t aht20_status_busy(uint8_t status)
{
    return (((status)&AHT20_STATUS_BUSY_MASK) >> AHT20_STATUS_BUSY_SHIFT);
}

#define AHT20_STATUS_MODE_SHIFT 5 // bit[6:5] Mode Status
#define AHT20_STATUS_MODE_MASK (0x3 << AHT20_STATUS_MODE_SHIFT)
// #define AHT20_STATUS_MODE(status) (((status) & AHT20_STATUS_MODE_MASK) >> AHT20_STATUS_MODE_SHIFT)
uint8_t aht20_status_mode(uint8_t status)
{
    return (((status)&AHT20_STATUS_MODE_MASK) >> AHT20_STATUS_MODE_SHIFT);
}
// bit[4] Reserved
#define AHT20_STATUS_CALI_SHIFT 3 // bit[3] CAL Enable
#define AHT20_STATUS_CALI_MASK (0x1 << AHT20_STATUS_CALI_SHIFT)
// bit[2:0] Reserved
// #define AHT20_STATUS_CALI(status) (((status) & AHT20_STATUS_CALI_MASK) >> AHT20_STATUS_CALI_SHIFT)
uint8_t aht20_status_cali(uint8_t status)
{
    return (((status)&AHT20_STATUS_CALI_MASK) >> AHT20_STATUS_CALI_SHIFT);
}

#define AHT20_STATUS_RESPONSE_MAX (6)
#define AHT20_RESOLUTION (1 << 20) // 2^20
#define AHT20_MAX_RETRY (10)

// 发送获取状态命令
static uint32_t AHT20_StatusCommand(void)
{
    uint8_t statusCmd[] = {AHT20_CMD_STATUS};
    // printf("AHT20_StatusCommand: send [0x%X]\n", statusCmd[0]);
    return AHT20_Write(statusCmd, sizeof(statusCmd));
}

// 发送软复位命令
static uint32_t AHT20_ResetCommand(void)
{
    uint8_t resetCmd[] = {AHT20_CMD_RESET};
    return AHT20_Write(resetCmd, sizeof(resetCmd));
}

// 发送初始化校准命令
static uint32_t AHT20_CalibrateCommand(void)
{
    uint8_t clibrateCmd[] = {AHT20_CMD_CALIBRATION, AHT20_CMD_CALIBRATION_ARG0, AHT20_CMD_CALIBRATION_ARG1};
    return AHT20_Write(clibrateCmd, sizeof(clibrateCmd));
}

// 读取温湿度值之前， 首先要看状态字的校准使能位Bit[3]是否为 1(通过发送0x71可以获取一个字节的状态字)，
// 如果不为1，要发送0xBE命令(初始化)，此命令参数有两个字节， 第一个字节为0x08，第二个字节为0x00。
uint32_t AHT20_Calibrate(void)
{
    uint32_t ret = 0;
    uint8_t buffer[AHT20_STATUS_RESPONSE_MAX]; // 6
    memset_s(&buffer, sizeof(buffer), 0x0, sizeof(buffer));

    ret = AHT20_StatusCommand();
    if (ret != 0)
    {
        return ret;
    }

    ret = AHT20_Read(buffer, sizeof(buffer));
    if (ret != 0)
    {
        return ret;
    }

    if (aht20_status_busy(buffer[0]) || !aht20_status_cali(buffer[0]))
    {
        ret = AHT20_ResetCommand();
        if (ret != 0)
        {
            return ret;
        }
        usleep(AHT20_STARTUP_TIME);
        ret = AHT20_CalibrateCommand();
        usleep(AHT20_CALIBRATION_TIME);
        return ret;
    }

    return 0;
}

// 发送 触发测量 命令，开始测量
uint32_t AHT20_StartMeasure(void)
{
    uint8_t triggerCmd[] = {AHT20_CMD_TRIGGER, AHT20_CMD_TRIGGER_ARG0, AHT20_CMD_TRIGGER_ARG1};
    return AHT20_Write(triggerCmd, sizeof(triggerCmd));
}

float tem_hum_get_data(float *temp, float *humi)
{
    AHT20_Calibrate();
    AHT20_StartMeasure();
    uint32_t humiRaw = 0;
    uint32_t tempRaw = 0;
    uint32_t ret = 0, i = 0;
    if (temp == NULL || humi == NULL)
    {
        return -1;
    }

    uint8_t buffer[AHT20_STATUS_RESPONSE_MAX]; // 6
    memset_s(&buffer, sizeof(buffer), 0x0, sizeof(buffer));
    ret = AHT20_Read(buffer, sizeof(buffer)); // recv status command result
    if (ret != 0)
    {
        return ret;
    }

    for (i = 0; aht20_status_busy(buffer[0]) && i < AHT20_MAX_RETRY; i++)
    {
        // printf("AHT20 device busy, retry %d/%d!\r\n", i, AHT20_MAX_RETRY);
        usleep(AHT20_MEASURE_TIME);
        ret = AHT20_Read(buffer, sizeof(buffer)); // recv status command result
        if (ret != 0)
        {
            return ret;
        }
    }
    if (i >= AHT20_MAX_RETRY)
    {
        printf("AHT20 device always busy!\r\n");
        return 0;
    }
    // printf("AHT20_GetMeasureResult: AHT20_Read[0x%02X][0x%02X][0x%02X][0x%02X][0x%02X][0x%02X]\n",
    // buffer[0],buffer[1],buffer[2],buffer[3],buffer[4],buffer[5]);

    humiRaw = (buffer[1] << 12) | (buffer[2] << 4) | (buffer[3] >> 4);
    tempRaw = ((buffer[3] & 0x0F) << 16) | (buffer[4] << 8) | (buffer[5]);

    *humi = humiRaw / (float)AHT20_RESOLUTION * 100;
    *temp = tempRaw / (float)AHT20_RESOLUTION * 200 - 50;

    // printf("AHT20_GetMeasureResult: humi[0x%05X][%d]->[%f], temp[0x%05X][%d]->[%f]\n",
    //         humiRaw, humiRaw, *humi, tempRaw, tempRaw, *temp);
    return 0;
}