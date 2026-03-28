/**
 * AI Biodome — ESP-IDF Main Entry
 *
 * Smart greenhouse monitoring system with:
 *  - SHT40 / BH1750 / SGP30 sensors via I2C
 *  - Soil moisture ADC
 *  - 4x relay control (pump, light, heater, fan)
 *  - WS2812 RGB LED status indicator
 *  - WiFi AP/STA with NVS persistence
 *  - HTTP server (port 80) + WebSocket (port 81)
 *  - Auto control logic with scheduling
 *  - 60-point history ring buffer
 *  - NTP time sync
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"

#include "nvs_storage.h"
#include "led_indicator.h"
#include "wifi_manager.h"
#include "sensors.h"
#include "relays.h"
#include "scheduler.h"
#include "history.h"
#include "http_server.h"
#include "ws_server.h"

static const char *TAG = "main";

/* ---- Shared global state ---- */
float    g_temperature = 0.0f;
float    g_humidity = 0.0f;
float    g_lux = 0.0f;
int      g_soil_percent = 0;
uint16_t g_eco2 = 400;
uint16_t g_tvoc = 0;
bool     g_auto_mode = true;

/* Critical section spinlock for global variable access */
portMUX_TYPE g_sensor_mux = portMUX_INITIALIZER_UNLOCKED;

static httpd_handle_t s_http_server = NULL;
static httpd_handle_t s_ws_server = NULL;

/* ---- Timer callbacks ---- */
static void sensor_timer_cb(TimerHandle_t timer)
{
    /* Read all sensors */
    sensor_data_t data;
    if (sensors_read_all(&data) == ESP_OK) {
        portENTER_CRITICAL(&g_sensor_mux);
        g_temperature = data.temperature;
        g_humidity = data.humidity;
        g_lux = data.lux;
        g_soil_percent = data.soil_percent;
        portEXIT_CRITICAL(&g_sensor_mux);
    }

    /* Snapshot eco2/tvoc for history and LED check */
    uint16_t eco2, tvoc;
    portENTER_CRITICAL(&g_sensor_mux);
    eco2 = g_eco2;
    tvoc = g_tvoc;
    portEXIT_CRITICAL(&g_sensor_mux);

    /* Run auto control logic */
    scheduler_run();

    /* Check 4-hour CSV auto export */
    scheduler_check_csv_export();

    /* Record history */
    history_record(g_temperature, g_humidity, g_lux, g_soil_percent, eco2, tvoc);

    /* Update LED indicator */
    if (g_soil_percent < 20 || g_temperature > 35.0f || eco2 > 1500) {
        led_set_color(255, 0, 0);  // Red = alert
    } else {
        led_set_color(0, 255, 0);  // Green = normal
    }

    /* Broadcast to WebSocket clients */
    if (wifi_is_connected() && s_ws_server) {
        char *json = ws_build_full_json();
        if (json) {
            ws_broadcast_json(json);
            free(json);
        }
    }
}

static void sgp30_timer_cb(TimerHandle_t timer)
{
    scheduler_sgp30_tick();
}

/* ---- NTP init ---- */
static void ntp_init(void)
{
    setenv("TZ", "CST-8", 1);
    tzset();

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_setservername(1, "time.nist.gov");
    esp_sntp_init();

    /* Wait for time sync (max 10 seconds) */
    time_t now = 0;
    struct tm timeinfo = {0};
    int retry = 0;
    while (timeinfo.tm_year < (2024 - 1900) && retry < 10) {
        ESP_LOGI(TAG, "Waiting for NTP sync... (%d)", retry);
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        localtime_r(&now, &timeinfo);
        retry++;
    }
    if (timeinfo.tm_year >= (2024 - 1900)) {
        ESP_LOGI(TAG, "NTP time synced: %04d-%02d-%02d %02d:%02d:%02d",
                 timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        ESP_LOGW(TAG, "NTP sync timeout, time may be incorrect");
    }
}

/* ---- app_main ---- */
void app_main(void)
{
    ESP_LOGI(TAG, "=== AI-Biodome Starting ===");
    ESP_LOGI(TAG, "ESP-IDF %s, %d cores, %d MHz",
             esp_get_idf_version(), 2, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);

    /* 1. Initialize NVS */
    nvs_init();

    /* 2. Initialize LED indicator */
    led_indicator_init();
    led_set_color(0, 0, 0);

    /* 3. Initialize relays (OFF by default) */
    relays_init();

    /* 4. Initialize I2C sensors */
    sensors_init();

    /* 5. Initialize history buffer */
    history_init();

    /* 6. Load schedule config */
    scheduler_init();

    /* 7. Load WiFi config and try STA connection */
    char ssid[33] = {0};
    char password[65] = {0};
    bool sta_connected = false;

    if (nvs_load_wifi(ssid, password) == ESP_OK && ssid[0] != '\0') {
        ESP_LOGI(TAG, "Attempting STA connection to: %s", ssid);
        sta_connected = wifi_init_sta(ssid, password);
    }

    if (sta_connected) {
        /* STA mode — full functionality */
        ESP_LOGI(TAG, "WiFi connected! IP: %s", wifi_get_ip_str());
        led_set_color(0, 255, 0);  // Green

        /* Init NTP */
        ntp_init();

        /* Start HTTP server (port 80) */
        s_http_server = http_server_start();

        /* Start WebSocket server (port 81) */
        s_ws_server = ws_server_start();

    } else {
        /* AP mode — configuration portal */
        ESP_LOGW(TAG, "Starting AP mode for WiFi configuration");
        led_set_color(255, 100, 0);  // Orange
        wifi_init_ap();

        /* Start HTTP server with AP config page */
        s_http_server = http_server_start();
    }

    /* 8. Create periodic timers */
    /* Sensor read + broadcast: every 5 seconds */
    TimerHandle_t sensor_timer = xTimerCreate(
        "sensor_timer", pdMS_TO_TICKS(5000), pdTRUE, NULL, sensor_timer_cb);
    xTimerStart(sensor_timer, 0);

    /* SGP30 tick: every 1 second (required for calibration) */
    TimerHandle_t sgp30_timer = xTimerCreate(
        "sgp30_timer", pdMS_TO_TICKS(1000), pdTRUE, NULL, sgp30_timer_cb);
    xTimerStart(sgp30_timer, 0);

    ESP_LOGI(TAG, "=== AI-Biodome Ready ===");
}
