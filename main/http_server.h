#pragma once
#include "esp_err.h"
#include "esp_http_server.h"

/**
 * Start HTTP server on port 80.
 */
httpd_handle_t http_server_start(void);

/**
 * Stop HTTP server.
 */
void http_server_stop(httpd_handle_t server);
