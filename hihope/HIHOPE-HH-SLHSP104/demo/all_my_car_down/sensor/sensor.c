#include "sensor.h"

/**
 * @brief 读取传感器数据
 * @param data 传感器数据结构体指针
 * @retval 无
 */
void SensorReadData(SensorData *data)
{
    data->illumination = GET_LIGHT();
    data->smoke = GET_SMOKER();
    data->pm25 = GP2Y1014AU();
    float BMEDATA[3] = {0};
    Get_BME280_Value(BMEDATA);
    data->temperature=BMEDATA[0];
    data->humidity=BMEDATA[1];
    data->pressure=BMEDATA[2];
    data->longitude=0;
    data->latitude=0;
    data->battery=0;
    // data->BMEDATA = BMEDATA;
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

        cJSON_AddNumberToObject(pro_obj, "humidity", data.humidity);
        cJSON_AddNumberToObject(pro_obj, "temperature", data.temperature);
        cJSON_AddNumberToObject(pro_obj, "pressure", data.pressure);
        cJSON_AddNumberToObject(pro_obj, "longitude", data.longitude);
        cJSON_AddNumberToObject(pro_obj, "latitude", data.latitude);
        cJSON_AddNumberToObject(pro_obj, "battery", data.battery);

        // 添加BME数据数组
        // cJSON *bme_array = cJSON_AddArrayToObject(pro_obj, "bme");
        // for (int i = 0; i < 3; i++) {
        //     cJSON *num = cJSON_CreateNumber(data.BMEDATA[i]);
        //     cJSON_AddItemToArray(bme_array, num);
        // }

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
}
