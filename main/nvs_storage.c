#include "nvs_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "nvs_storage";

esp_err_t nvs_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_LOGI(TAG, "NVS initialized: %s", esp_err_to_name(ret));
    return ret;
}

esp_err_t nvs_save_wifi(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("wifi-config", NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    nvs_set_str(handle, "ssid", ssid);
    nvs_set_str(handle, "password", password);
    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG, "WiFi config saved: SSID=%s", ssid);
    return ESP_OK;
}

esp_err_t nvs_load_wifi(char *ssid, char *password)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("wifi-config", NVS_READONLY, &handle);
    if (ret != ESP_OK) return ret;

    size_t len = 33;
    ret = nvs_get_str(handle, "ssid", ssid, &len);
    if (ret != ESP_OK) {
        nvs_close(handle);
        return ret;
    }

    len = 65;
    ret = nvs_get_str(handle, "password", password, &len);
    nvs_close(handle);

    if (strlen(ssid) == 0) return ESP_FAIL;
    return ret;
}

esp_err_t nvs_clear_wifi(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("wifi-config", NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    nvs_erase_all(handle);
    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG, "WiFi config cleared");
    return ESP_OK;
}

esp_err_t nvs_save_sched(bool fan_en, const char *fan_start, const char *fan_end,
                          bool light_en, const char *light_start, const char *light_end)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("sched", NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    nvs_set_u8(handle, "f_en", fan_en ? 1 : 0);
    nvs_set_str(handle, "f_s", fan_start);
    nvs_set_str(handle, "f_e", fan_end);
    nvs_set_u8(handle, "l_en", light_en ? 1 : 0);
    nvs_set_str(handle, "l_s", light_start);
    nvs_set_str(handle, "l_e", light_end);
    nvs_commit(handle);
    nvs_close(handle);
    ESP_LOGI(TAG, "Schedule config saved");
    return ESP_OK;
}

esp_err_t nvs_load_sched(bool *fan_en, char *fan_start, char *fan_end,
                          bool *light_en, char *light_start, char *light_end)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("sched", NVS_READONLY, &handle);
    if (ret != ESP_OK) return ret;

    uint8_t val = 0;
    size_t len;

    if (nvs_get_u8(handle, "f_en", &val) == ESP_OK) *fan_en = (val != 0);
    len = 6;  nvs_get_str(handle, "f_s", fan_start, &len);
    len = 6;  nvs_get_str(handle, "f_e", fan_end, &len);

    val = 0;
    if (nvs_get_u8(handle, "l_en", &val) == ESP_OK) *light_en = (val != 0);
    len = 6;  nvs_get_str(handle, "l_s", light_start, &len);
    len = 6;  nvs_get_str(handle, "l_e", light_end, &len);

    nvs_close(handle);
    return ESP_OK;
}

esp_err_t nvs_save_csv_export_time(uint32_t timestamp)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("csv-export", NVS_READWRITE, &handle);
    if (ret != ESP_OK) return ret;

    nvs_set_u32(handle, "last_ts", timestamp);
    nvs_commit(handle);
    nvs_close(handle);
    return ESP_OK;
}

uint32_t nvs_load_csv_export_time(void)
{
    nvs_handle_t handle;
    esp_err_t ret = nvs_open("csv-export", NVS_READONLY, &handle);
    if (ret != ESP_OK) return 0;

    uint32_t ts = 0;
    nvs_get_u32(handle, "last_ts", &ts);
    nvs_close(handle);
    return ts;
}
