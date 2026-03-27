#pragma once
#include "esp_err.h"
#include "esp_wifi.h"
#include <stdbool.h>

#define WIFI_AP_SSID "AI-Biodome-Config"
#define WIFI_STA_TIMEOUT_SEC 30

/**
 * Initialize WiFi in STA mode with given credentials.
 * Blocks until connected or timeout, then falls back to AP.
 * @return true if STA connected, false if fell back to AP
 */
bool wifi_init_sta(const char *ssid, const char *password);

/**
 * Initialize WiFi in AP mode for configuration.
 */
void wifi_init_ap(void);

/**
 * Check if WiFi is connected in STA mode.
 */
bool wifi_is_connected(void);

/**
 * Get current RSSI.
 */
int wifi_get_rssi(void);

/**
 * Get IP string (STA or AP).
 */
const char *wifi_get_ip_str(void);

/**
 * Get MAC address string.
 */
const char *wifi_get_mac_str(void);
