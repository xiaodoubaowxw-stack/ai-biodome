#include "wifi_manager.h"
#include "led_indicator.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "wifi";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static bool s_connected = false;
static char s_ip_str[16] = "0.0.0.0";
static char s_mac_str[18] = "00:00:00:00:00:00";
static esp_netif_t *s_netif_sta = NULL;
static bool s_wifi_led_blinking = false;  /* flag: LED blink requested by event handler */

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static void event_handler(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        if (s_retry_num < WIFI_STA_TIMEOUT_SEC) {
            s_retry_num++;
            ESP_LOGW(TAG, "WiFi disconnected, retry %d/%d", s_retry_num, WIFI_STA_TIMEOUT_SEC);
            /* Signal LED blink — handled by a separate task/timer, not here */
            s_wifi_led_blinking = true;
            esp_wifi_connect();
        } else {
            ESP_LOGW(TAG, "WiFi STA connection failed, giving up");
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&event->ip_info.ip));
        s_connected = true;
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static bool s_initialized = false;

static void init_common(void)
{
    if (s_initialized) return;

    s_wifi_event_group = xEventGroupCreate();

    esp_err_t ret = esp_netif_init();
    if (ret != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(ret);
    ret = esp_event_loop_create_default();
    if (ret != ESP_ERR_INVALID_STATE) ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

    s_initialized = true;
}

bool wifi_init_sta(const char *ssid, const char *password)
{
    init_common();

    s_netif_sta = esp_netif_create_default_wifi_sta();

    wifi_config_t wifi_cfg = {0};
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    strncpy((char *)wifi_cfg.sta.password, password, sizeof(wifi_cfg.sta.password) - 1);
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi STA started, connecting to %s ...", ssid);

    /* Wait for connection or failure */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(WIFI_STA_TIMEOUT_SEC * 2000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to %s, IP: %s", ssid, s_ip_str);
        /* Get MAC */
        uint8_t mac[6];
        esp_wifi_get_mac(WIFI_IF_STA, mac);
        snprintf(s_mac_str, sizeof(s_mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return true;
    }

    ESP_LOGW(TAG, "WiFi STA connection timed out, falling back to AP");
    esp_wifi_stop();
    return false;
}

void wifi_init_ap(void)
{
    if (s_wifi_event_group == NULL) {
        init_common();
    }

    esp_netif_t *netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = WIFI_AP_SSID,
            .ssid_len = strlen(WIFI_AP_SSID),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_OPEN,
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    /* Get AP IP */
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(netif_ap, &ip_info);
    snprintf(s_ip_str, sizeof(s_ip_str), IPSTR, IP2STR(&ip_info.ip));

    /* Get MAC */
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_AP, mac);
    snprintf(s_mac_str, sizeof(s_mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    ESP_LOGI(TAG, "WiFi AP started: SSID=%s, IP=%s", WIFI_AP_SSID, s_ip_str);
}

bool wifi_is_connected(void)
{
    return s_connected;
}

int wifi_get_rssi(void)
{
    if (!s_connected) return 0;
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return ap_info.rssi;
    }
    return 0;
}

const char *wifi_get_ip_str(void)
{
    return s_ip_str;
}

const char *wifi_get_mac_str(void)
{
    return s_mac_str;
}
