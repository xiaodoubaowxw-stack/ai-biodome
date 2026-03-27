#include "history.h"
#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "history";
static history_t s_history;

void history_init(void)
{
    memset(&s_history, 0, sizeof(s_history));
    ESP_LOGI(TAG, "History buffer initialized (%d entries)", HISTORY_SIZE);
}

void history_record(float temp, float hum, float lux, int soil,
                    uint16_t eco2, uint16_t tvoc)
{
    char time_str[12] = "00:00:00";
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year > (2020 - 1900)) {
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        snprintf(time_str, sizeof(time_str), "%lus",
                 (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000));
    }

    if (s_history.count < HISTORY_SIZE) {
        int idx = s_history.count;
        s_history.temp[idx] = temp;
        s_history.hum[idx] = hum;
        s_history.lux[idx] = lux;
        s_history.soil[idx] = soil;
        s_history.eco2[idx] = eco2;
        s_history.tvoc[idx] = tvoc;
        strncpy(s_history.time[idx], time_str, 11);
        s_history.count++;
    } else {
        int idx = s_history.head;
        s_history.temp[idx] = temp;
        s_history.hum[idx] = hum;
        s_history.lux[idx] = lux;
        s_history.soil[idx] = soil;
        s_history.eco2[idx] = eco2;
        s_history.tvoc[idx] = tvoc;
        strncpy(s_history.time[idx], time_str, 11);
        s_history.head = (s_history.head + 1) % HISTORY_SIZE;
    }
}

const history_t *history_get(void)
{
    return &s_history;
}

void *history_build_json(void)
{
    cJSON *time_arr = cJSON_CreateArray();
    cJSON *temp_arr = cJSON_CreateArray();
    cJSON *hum_arr = cJSON_CreateArray();
    cJSON *lux_arr = cJSON_CreateArray();
    cJSON *soil_arr = cJSON_CreateArray();
    cJSON *eco2_arr = cJSON_CreateArray();
    cJSON *tvoc_arr = cJSON_CreateArray();

    for (int i = 0; i < s_history.count; i++) {
        int idx = (s_history.head + i) % HISTORY_SIZE;
        cJSON_AddItemToArray(time_arr, cJSON_CreateString(s_history.time[idx]));
        cJSON_AddItemToArray(temp_arr, cJSON_CreateNumber(s_history.temp[idx]));
        cJSON_AddItemToArray(hum_arr, cJSON_CreateNumber(s_history.hum[idx]));
        cJSON_AddItemToArray(lux_arr, cJSON_CreateNumber(s_history.lux[idx]));
        cJSON_AddItemToArray(soil_arr, cJSON_CreateNumber(s_history.soil[idx]));
        cJSON_AddItemToArray(eco2_arr, cJSON_CreateNumber(s_history.eco2[idx]));
        cJSON_AddItemToArray(tvoc_arr, cJSON_CreateNumber(s_history.tvoc[idx]));
    }

    cJSON *history = cJSON_CreateObject();
    cJSON_AddItemToObject(history, "time", time_arr);
    cJSON_AddItemToObject(history, "temp", temp_arr);
    cJSON_AddItemToObject(history, "hum", hum_arr);
    cJSON_AddItemToObject(history, "lux", lux_arr);
    cJSON_AddItemToObject(history, "soil", soil_arr);
    cJSON_AddItemToObject(history, "eco2", eco2_arr);
    cJSON_AddItemToObject(history, "tvoc", tvoc_arr);

    return history;
}
