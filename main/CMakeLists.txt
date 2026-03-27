idf_component_register(
    SRCS "main.c" "wifi_manager.c" "http_server.c" "ws_server.c"
         "sensors.c" "relays.c" "scheduler.c" "nvs_storage.c"
         "led_indicator.c" "history.c"
    INCLUDE_DIRS "."
    REQUIRES driver nvs_flash esp_wifi esp_netif esp_http_server esp_timer json led_strip
)
# Embed HTML page
target_add_binary_data(${COMPONENT_LIB} "webpage.h" TEXT)
