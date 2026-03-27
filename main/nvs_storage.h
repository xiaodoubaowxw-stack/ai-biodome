#pragma once
#include "esp_err.h"
#include <stdbool.h>

/**
 * Initialize NVS flash storage.
 */
esp_err_t nvs_init(void);

/**
 * Save WiFi credentials to NVS.
 */
esp_err_t nvs_save_wifi(const char *ssid, const char *password);

/**
 * Load WiFi credentials from NVS.
 * @param ssid Output buffer (must be at least 33 bytes)
 * @param password Output buffer (must be at least 65 bytes)
 * @return ESP_OK if credentials found, ESP_FAIL otherwise
 */
esp_err_t nvs_load_wifi(char *ssid, char *password);

/**
 * Clear WiFi credentials from NVS.
 */
esp_err_t nvs_clear_wifi(void);

/**
 * Save schedule configuration.
 */
esp_err_t nvs_save_sched(bool fan_en, const char *fan_start, const char *fan_end,
                          bool light_en, const char *light_start, const char *light_end);

/**
 * Load schedule configuration.
 */
esp_err_t nvs_load_sched(bool *fan_en, char *fan_start, char *fan_end,
                          bool *light_en, char *light_start, char *light_end);
