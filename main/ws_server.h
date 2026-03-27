#pragma once
#include "esp_err.h"
#include "esp_http_server.h"

/**
 * Start WebSocket server on port 81.
 */
httpd_handle_t ws_server_start(void);

/**
 * Stop WebSocket server.
 */
void ws_server_stop(httpd_handle_t server);

/**
 * Broadcast JSON data to all connected WebSocket clients.
 */
void ws_broadcast_json(const char *json_str);

/**
 * Send state-only JSON to all clients.
 */
void ws_broadcast_state(void);

/**
 * Build and return full JSON data string.
 * The caller must free() the returned string.
 */
char *ws_build_full_json(void);
