#pragma once
#include "esp_err.h"
#include <stdint.h>

/**
 * Initialize WS2812 RGB LED on GPIO 48.
 */
esp_err_t led_indicator_init(void);

/**
 * Set LED color.
 */
void led_set_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * Turn off LED.
 */
void led_clear(void);
