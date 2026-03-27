#include "ws_server.h"
#include "sensors.h"
#include "relays.h"
#include "history.h"
#include "scheduler.h"
#include "nvs_storage.h"
#include "wifi_manager.h"
#include "cJSON.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

static const char *TAG = "ws_server";

/* Shared state (declared in main.c, accessed via extern or scheduler) */
extern float g_temperature, g_humidity, g_lux;
extern int g_soil_percent;
extern uint16_t g_eco2, g_tvoc;
extern bool g_auto_mode;

static httpd_handle_t s_ws_server = NULL;
static int s_ws_clients[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
static int s_ws_client_count = 0;

static void add_client(int fd)
{
    for (int i = 0; i < 8; i++) {
        if (s_ws_clients[i] == -1) {
            s_ws_clients[i] = fd;
            s_ws_client_count++;
            return;
        }
    }
}

static void remove_client(int fd)
{
    for (int i = 0; i < 8; i++) {
        if (s_ws_clients[i] == fd) {
            s_ws_clients[i] = -1;
            s_ws_client_count--;
            return;
        }
    }
}

char *ws_build_full_json(void)
{
    cJSON *root = cJSON_CreateObject();

    /* Current sensor data */
    cJSON *current = cJSON_CreateObject();
    cJSON_AddNumberToObject(current, "temp", g_temperature);
    cJSON_AddNumberToObject(current, "hum", g_humidity);
    cJSON_AddNumberToObject(current, "lux", g_lux);
    cJSON_AddNumberToObject(current, "soil", g_soil_percent);
    cJSON_AddNumberToObject(current, "eco2", g_eco2);
    cJSON_AddNumberToObject(current, "tvoc", g_tvoc);
    cJSON_AddItemToObject(root, "current", current);

    /* Device state */
    cJSON *state = cJSON_CreateObject();
    cJSON_AddStringToObject(state, "mode", g_auto_mode ? "auto" : "manual");
    cJSON_AddNumberToObject(state, "pump", relay_get(RELAY_PUMP) ? 1 : 0);
    cJSON_AddNumberToObject(state, "light", relay_get(RELAY_LIGHT) ? 1 : 0);
    cJSON_AddNumberToObject(state, "heater", relay_get(RELAY_HEATER) ? 1 : 0);
    cJSON_AddNumberToObject(state, "fan", relay_get(RELAY_FAN) ? 1 : 0);
    cJSON_AddItemToObject(root, "state", state);

    /* Time */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year > (2020 - 1900)) {
        char time_str[10];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                 timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        cJSON_AddStringToObject(root, "time", time_str);
    }

    /* Schedule config */
    const sched_config_t *sc = scheduler_get_config();
    cJSON *sched = cJSON_CreateObject();
    cJSON_AddBoolToObject(sched, "fan_en", sc->fan_en);
    cJSON_AddStringToObject(sched, "fan_start", sc->fan_start);
    cJSON_AddStringToObject(sched, "fan_end", sc->fan_end);
    cJSON_AddBoolToObject(sched, "light_en", sc->light_en);
    cJSON_AddStringToObject(sched, "light_start", sc->light_start);
    cJSON_AddStringToObject(sched, "light_end", sc->light_end);
    cJSON_AddItemToObject(root, "sched", sched);

    /* System info */
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddNumberToObject(sys, "heap_total", esp_get_free_heap_size() + 50000);  // approximate
    cJSON_AddNumberToObject(sys, "heap_free", esp_get_free_heap_size());
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddNumberToObject(sys, "chip_rev", chip_info.revision);
    cJSON_AddNumberToObject(sys, "cpu_cores", chip_info.cores);
    cJSON_AddStringToObject(sys, "mac", wifi_get_mac_str());
    cJSON_AddStringToObject(sys, "sdk_ver", esp_get_idf_version());
    cJSON_AddNumberToObject(sys, "rssi", wifi_get_rssi());
    cJSON_AddStringToObject(sys, "ip", wifi_get_ip_str());
    cJSON_AddNumberToObject(sys, "cpu_freq", CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    cJSON_AddNumberToObject(sys, "uptime",
        (unsigned long)(xTaskGetTickCount() * portTICK_PERIOD_MS / 1000));
    cJSON_AddItemToObject(root, "system", sys);

    /* History */
    cJSON *hist = history_build_json();
    cJSON_AddItemToObject(root, "history", hist);

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}

static cJSON *build_state_json(void)
{
    cJSON *root = cJSON_CreateObject();
    cJSON *state = cJSON_CreateObject();
    cJSON_AddStringToObject(state, "mode", g_auto_mode ? "auto" : "manual");
    cJSON_AddNumberToObject(state, "pump", relay_get(RELAY_PUMP) ? 1 : 0);
    cJSON_AddNumberToObject(state, "light", relay_get(RELAY_LIGHT) ? 1 : 0);
    cJSON_AddNumberToObject(state, "heater", relay_get(RELAY_HEATER) ? 1 : 0);
    cJSON_AddNumberToObject(state, "fan", relay_get(RELAY_FAN) ? 1 : 0);
    cJSON_AddItemToObject(root, "state", state);
    return root;
}

void ws_broadcast_json(const char *json_str)
{
    if (!s_ws_server || !json_str) return;
    httpd_ws_frame_t ws_pkt = {
        .payload = (uint8_t *)json_str,
        .len = strlen(json_str),
        .type = HTTPD_WS_TYPE_TEXT,
    };
    for (int i = 0; i < 8; i++) {
        if (s_ws_clients[i] != -1) {
            httpd_ws_send_frame_async(s_ws_server, s_ws_clients[i], &ws_pkt);
        }
    }
}

void ws_broadcast_state(void)
{
    cJSON *root = build_state_json();
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    ws_broadcast_json(json_str);
    free(json_str);
}

/* Handle incoming WebSocket messages */
static void handle_ws_message(const uint8_t *payload, size_t len)
{
    cJSON *root = cJSON_ParseWithLength((const char *)payload, len);
    if (!root) {
        ESP_LOGW(TAG, "Invalid JSON received");
        return;
    }

    cJSON *action = cJSON_GetObjectItem(root, "action");
    if (!action || !cJSON_IsString(action)) {
        cJSON_Delete(root);
        return;
    }

    const char *action_str = action->valuestring;

    if (strcmp(action_str, "set_mode") == 0) {
        cJSON *mode = cJSON_GetObjectItem(root, "mode");
        if (mode && cJSON_IsString(mode)) {
            g_auto_mode = (strcmp(mode->valuestring, "auto") == 0);
            scheduler_run();
            ws_broadcast_state();
        }
    }
    else if (strcmp(action_str, "set_device") == 0 && !g_auto_mode) {
        cJSON *device = cJSON_GetObjectItem(root, "device");
        cJSON *state_val = cJSON_GetObjectItem(root, "state");
        if (device && cJSON_IsString(device) && state_val) {
            bool is_on = cJSON_IsNumber(state_val) ? (state_val->valueint == 1) : false;
            const char *dev = device->valuestring;
            if (strcmp(dev, "pump") == 0) relay_set(RELAY_PUMP, is_on);
            else if (strcmp(dev, "light") == 0) relay_set(RELAY_LIGHT, is_on);
            else if (strcmp(dev, "heater") == 0) relay_set(RELAY_HEATER, is_on);
            else if (strcmp(dev, "fan") == 0) relay_set(RELAY_FAN, is_on);
            ws_broadcast_state();
        }
    }
    else if (strcmp(action_str, "set_sched") == 0) {
        sched_config_t *sc = scheduler_get_config_mutable();
        cJSON *fan_en = cJSON_GetObjectItem(root, "fan_en");
        cJSON *fan_s = cJSON_GetObjectItem(root, "fan_start");
        cJSON *fan_e = cJSON_GetObjectItem(root, "fan_end");
        cJSON *light_en = cJSON_GetObjectItem(root, "light_en");
        cJSON *light_s = cJSON_GetObjectItem(root, "light_start");
        cJSON *light_e = cJSON_GetObjectItem(root, "light_end");

        if (fan_en) sc->fan_en = cJSON_IsTrue(fan_en);
        if (fan_s && cJSON_IsString(fan_s)) strncpy(sc->fan_start, fan_s->valuestring, 5);
        if (fan_e && cJSON_IsString(fan_e)) strncpy(sc->fan_end, fan_e->valuestring, 5);
        if (light_en) sc->light_en = cJSON_IsTrue(light_en);
        if (light_s && cJSON_IsString(light_s)) strncpy(sc->light_start, light_s->valuestring, 5);
        if (light_e && cJSON_IsString(light_e)) strncpy(sc->light_end, light_e->valuestring, 5);

        nvs_save_sched(sc->fan_en, sc->fan_start, sc->fan_end,
                        sc->light_en, sc->light_start, sc->light_end);

        scheduler_run();
        ws_broadcast_state();

        /* Also send full data so schedule UI updates */
        char *full = ws_build_full_json();
        if (full) {
            ws_broadcast_json(full);
            free(full);
        }
    }

    cJSON_Delete(root);
}

/* WebSocket handler */
static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        int fd = httpd_req_to_sockfd(req);
        add_client(fd);
        ESP_LOGI(TAG, "WebSocket client connected (fd=%d, total=%d)", fd, s_ws_client_count);

        /* Send full data on connect */
        char *json_str = ws_build_full_json();
        if (json_str) {
            httpd_ws_frame_t ws_pkt = {
                .payload = (uint8_t *)json_str,
                .len = strlen(json_str),
                .type = HTTPD_WS_TYPE_TEXT,
            };
            httpd_ws_send_frame(req, &ws_pkt);
            free(json_str);
        }
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt = {0};
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    /* First call to get frame length */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "WebSocket recv frame failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (ws_pkt.len > 0) {
        ws_pkt.payload = malloc(ws_pkt.len + 1);
        if (!ws_pkt.payload) return ESP_ERR_NO_MEM;

        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret == ESP_OK) {
            ws_pkt.payload[ws_pkt.len] = '\0';
            handle_ws_message(ws_pkt.payload, ws_pkt.len);
        }
        free(ws_pkt.payload);
    }

    return ret;
}

/* Track client disconnect */
static esp_err_t ws_close_handler(httpd_req_t *req)
{
    int fd = httpd_req_to_sockfd(req);
    remove_client(fd);
    ESP_LOGI(TAG, "WebSocket client disconnected (fd=%d, total=%d)", fd, s_ws_client_count);
    return ESP_OK;
}

httpd_handle_t ws_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 81;
    config.max_uri_handlers = 4;
    config.stack_size = 8192;
    config.max_open_sockets = 8;

    s_ws_server = NULL;
    if (httpd_start(&s_ws_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WebSocket server on port 81");
        return NULL;
    }

    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = ws_handler,
        .is_websocket = true,
        .handle_ws_control_frames = false,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_ws_server, &ws_uri);

    /* Initialize client list */
    for (int i = 0; i < 8; i++) s_ws_clients[i] = -1;
    s_ws_client_count = 0;

    ESP_LOGI(TAG, "WebSocket server started on port 81");
    return s_ws_server;
}

void ws_server_stop(httpd_handle_t server)
{
    if (server) {
        httpd_stop(server);
        s_ws_server = NULL;
    }
}
