#include "scheduler.h"
#include "sensors.h"
#include "relays.h"
#include "nvs_storage.h"
#include "esp_log.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "scheduler";

static sched_config_t s_sched = {
    .fan_en = false,
    .fan_start = "00:00",
    .fan_end = "00:00",
    .light_en = false,
    .light_start = "00:00",
    .light_end = "00:00",
};

/* Shared state defined in main.c */
extern bool g_auto_mode;
extern float g_temperature, g_humidity, g_lux;
extern int g_soil_percent;
extern uint16_t g_eco2, g_tvoc;
extern portMUX_TYPE g_sensor_mux;

static bool is_time_in_range(const char *current, const char *start, const char *end)
{
    if (strcmp(start, end) == 0) return false;
    if (strcmp(start, end) < 0) {
        return (strcmp(current, start) >= 0 && strcmp(current, end) < 0);
    }
    /* Wrap around midnight */
    return (strcmp(current, start) >= 0 || strcmp(current, end) < 0);
}

static void get_current_time_str(char *buf, size_t len)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year > (2020 - 1900)) {
        snprintf(buf, len, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    } else {
        buf[0] = '\0';
    }
}

esp_err_t scheduler_init(void)
{
    nvs_load_sched(&s_sched.fan_en, s_sched.fan_start, s_sched.fan_end,
                    &s_sched.light_en, s_sched.light_start, s_sched.light_end);
    ESP_LOGI(TAG, "Scheduler initialized: fan=[%s %s-%s] light=[%s %s-%s]",
             s_sched.fan_en ? "ON" : "OFF", s_sched.fan_start, s_sched.fan_end,
             s_sched.light_en ? "ON" : "OFF", s_sched.light_start, s_sched.light_end);
    return ESP_OK;
}

void scheduler_run(void)
{
    if (!g_auto_mode) return;

    /* Snapshot shared variables under critical section */
    float temp, hum, lux;
    int soil;
    uint16_t eco2;
    portENTER_CRITICAL(&g_sensor_mux);
    temp = g_temperature;
    hum = g_humidity;
    lux = g_lux;
    soil = g_soil_percent;
    eco2 = g_eco2;
    portEXIT_CRITICAL(&g_sensor_mux);

    char cur_time[6] = "";
    get_current_time_str(cur_time, sizeof(cur_time));

    /* 1. Soil moisture → pump */
    if (soil < 30) {
        relay_set(RELAY_PUMP, true);
    } else if (soil > 60) {
        relay_set(RELAY_PUMP, false);
    }

    /* 2. Light control: sensor + schedule */
    bool light_by_sensor = (lux < 200.0f);
    bool light_by_sched = (s_sched.light_en && cur_time[0] &&
                           is_time_in_range(cur_time, s_sched.light_start, s_sched.light_end));
    bool state_light = light_by_sensor || light_by_sched;
    if (lux > 800.0f && !light_by_sched) state_light = false;
    relay_set(RELAY_LIGHT, state_light);

    /* 3. Heater: temp < 18 */
    bool state_heater = relay_get(RELAY_HEATER);
    if (temp < 18.0f) state_heater = true;
    else if (temp > 22.0f) state_heater = false;
    relay_set(RELAY_HEATER, state_heater);

    /* 4. Fan: high temp/high CO2 + schedule */
    bool fan_by_sensor = (temp > 28.0f || eco2 > 1000);
    bool fan_by_sched = (s_sched.fan_en && cur_time[0] &&
                         is_time_in_range(cur_time, s_sched.fan_start, s_sched.fan_end));
    bool state_fan = fan_by_sensor || fan_by_sched;
    if (temp < 25.0f && eco2 < 800 && !fan_by_sched) state_fan = false;
    relay_set(RELAY_FAN, state_fan);
}

void scheduler_sgp30_tick(void)
{
    /* SGP30 needs to be read every 1 second for internal calibration */
    uint16_t eco2, tvoc;
    if (sensor_read_sgp30(&eco2, &tvoc) == ESP_OK) {
        portENTER_CRITICAL(&g_sensor_mux);
        g_eco2 = eco2;
        g_tvoc = tvoc;
        portEXIT_CRITICAL(&g_sensor_mux);
    }
}

const sched_config_t *scheduler_get_config(void)
{
    return &s_sched;
}

sched_config_t *scheduler_get_config_mutable(void)
{
    return &s_sched;
}
