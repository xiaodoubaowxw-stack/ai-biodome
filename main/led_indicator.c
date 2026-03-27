#include "led_indicator.h"
#include "led_strip.h"
#include "esp_log.h"

static const char *TAG = "led";
static led_strip_handle_t s_led_strip = NULL;

esp_err_t led_indicator_init(void)
{
    /* LED strip general configuration */
    led_strip_config_t strip_config = {
        .strip_gpio_num = 48,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .flags = {
            .invert_out = false,
        },
    };

    /* LED strip RMT configuration */
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .flags = {
            .with_dma = false,
        },
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create LED strip: %s", esp_err_to_name(ret));
        return ret;
    }

    led_strip_clear(s_led_strip);
    ESP_LOGI(TAG, "WS2812 LED initialized on GPIO 48");
    return ESP_OK;
}

void led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_led_strip) return;
    led_strip_set_pixel(s_led_strip, 0, r, g, b);
    led_strip_refresh(s_led_strip);
}

void led_clear(void)
{
    if (!s_led_strip) return;
    led_strip_clear(s_led_strip);
}
