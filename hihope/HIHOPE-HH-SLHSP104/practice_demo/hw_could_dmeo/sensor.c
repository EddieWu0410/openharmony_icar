#include "sensor.h"

/**
 * @brief 读取传感器数据
 * @param data 传感器数据结构体指针
 * @retval 无
 */
void SensorReadData(SensorData *data)
{
    // data->illumination = GET_LIGHT();
    // data->smoke = GET_SMOKER();
    // data->pm25 = GP2Y1014AU();
    data->illumination = 20;
    data->smoke = 10;
    data->pm25 = 30;
}

/**
 * @brief 获取传感器数据Json字符串
 * @param payload Json字符串指针
 * @retval 无
 */
void get_sensor_public_string(char *payload)
{
    SensorData data;
    // 读取传感器数据
    SensorReadData(&data);

    // 创建 JSON 对象
    cJSON *root = cJSON_CreateObject();
    if (root != NULL)
    {
        cJSON *serv_arr = cJSON_AddArrayToObject(root, "services");
        cJSON *arr_item = cJSON_CreateObject();
        cJSON_AddStringToObject(arr_item, "service_id", "sensorData");
        cJSON *pro_obj = cJSON_CreateObject();
        cJSON_AddItemToObject(arr_item, "properties", pro_obj);
        cJSON_AddNumberToObject(pro_obj, "illumination", data.illumination);
        cJSON_AddNumberToObject(pro_obj, "smoke", data.smoke);
        cJSON_AddNumberToObject(pro_obj, "pm25", data.pm25);
        cJSON_AddItemToArray(serv_arr, arr_item);
        // 生成 JSON 字符串
        char *payload_str = cJSON_PrintUnformatted(root);
        if (payload_str != NULL)
        {
            strcpy(payload, payload_str);
            cJSON_free(payload_str);
        }
        // 删除 JSON 对象
        cJSON_Delete(root);
    }
    // --将读取到的传感器数据封装成上华为云IoT平台的数据结构--end
}