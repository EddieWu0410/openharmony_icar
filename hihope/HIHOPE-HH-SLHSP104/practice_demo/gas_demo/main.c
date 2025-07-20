#include "gas.h"

// 传感器数据采集线程
static void *Sensor_TASK(void *arg)
{
    uint16_t gas_value = 0;
    
    while (1) {
        // 获取可燃气体浓度值
        gas_value = gas_get_data();
        
        // 通过串口输出气体浓度百分比
        printf("gas_value：%d%%\n", gas_value);  // 注意输出格式
        
        // 200ms采样间隔
        osDelay(200);
    }
}

// 程序主入口
static void MAIN_Entry(void)
{
    // 初始化可燃气体传感器
    gas_init();  // 注意：调用gas_init()而非light_init()
    
    // 创建传感器采集线程
    osThreadAttr_t attr = {
        .name = "Sensor_TASK",
        .attr_bits = 0U,
        .cb_mem = NULL,
        .cb_size = 0U,
        .stack_mem = NULL,
        .stack_size = 1024 * 5,  // 5KB栈空间
        .priority = osPriorityNormal
    };
    
    if (osThreadNew(Sensor_TASK, NULL, &attr) == NULL) {
        printf("[MAIN_Entry] create Sensor_TASK NG\n");
    } else {
        printf("[MAIN_Entry] create Sensor_TASK OK\n");
    }
}

// 注册应用入口
APP_FEATURE_INIT(MAIN_Entry);