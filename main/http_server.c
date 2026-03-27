#include "http_server.h"
#include "nvs_storage.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include <string.h>
#include <stdlib.h>

/* Embedded HTML from webpage.h */
extern const uint8_t index_html_start[] asm("_binary_webpage_h_start");
extern const uint8_t index_html_end[]   asm("_binary_webpage_h_end");

static const char *TAG = "http_server";

/* ---- GET / ---- Serve dashboard ---- */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    const char *html = (const char *)index_html_start;
    size_t html_len = index_html_end - index_html_start;
    httpd_resp_send(req, html, html_len);
    return ESP_OK;
}

/* ---- POST /save ---- Save WiFi config ---- */
static esp_err_t save_post_handler(httpd_req_t *req)
{
    char buf[256] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No data");
        return ESP_FAIL;
    }

    /* Parse URL-encoded form: ssid=xxx&pass=xxx */
    char ssid[33] = {0};
    char pass[65] = {0};

    char *ssid_ptr = strstr(buf, "ssid=");
    char *pass_ptr = strstr(buf, "pass=");
    if (ssid_ptr) {
        ssid_ptr += 5;
        char *end = strchr(ssid_ptr, '&');
        if (end) {
            size_t len = end - ssid_ptr;
            if (len > 32) len = 32;
            strncpy(ssid, ssid_ptr, len);
        } else {
            strncpy(ssid, ssid_ptr, 32);
        }
    }
    if (pass_ptr) {
        pass_ptr += 5;
        char *end = strchr(pass_ptr, '&');
        if (end) {
            size_t len = end - pass_ptr;
            if (len > 64) len = 64;
            strncpy(pass, pass_ptr, len);
        } else {
            strncpy(pass, pass_ptr, 64);
        }
    }

    ESP_LOGI(TAG, "Saving WiFi config: SSID=%s", ssid);
    nvs_save_wifi(ssid, pass);

    const char *resp = "<html><head><meta charset='UTF-8'></head>"
                       "<body><h2>配置已保存！设备正在重启...</h2></body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    return ESP_OK;
}

/* ---- GET /reset ---- Clear WiFi config ---- */
static esp_err_t reset_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Clearing WiFi config and restarting");
    nvs_clear_wifi();

    const char *resp = "<html><head><meta charset='UTF-8'></head>"
                       "<body><h2>WiFi配置已清除！重启中...</h2></body></html>";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    return ESP_OK;
}

httpd_handle_t http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.max_uri_handlers = 8;
    config.stack_size = 8192;

    httpd_handle_t server = NULL;
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }

    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_get_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &root_uri);

    httpd_uri_t save_uri = {
        .uri = "/save",
        .method = HTTP_POST,
        .handler = save_post_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &save_uri);

    httpd_uri_t reset_uri = {
        .uri = "/reset",
        .method = HTTP_GET,
        .handler = reset_get_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &reset_uri);

    ESP_LOGI(TAG, "HTTP server started on port 80");
    return server;
}

void http_server_stop(httpd_handle_t server)
{
    if (server) {
        httpd_stop(server);
    }
}
