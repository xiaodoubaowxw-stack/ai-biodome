#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#define RELAY_PUMP    0
#define RELAY_LIGHT   1
#define RELAY_HEATER  2
#define RELAY_FAN     3
#define RELAY_COUNT   4

#define RELAY_ON  0  // Low-level trigger
#define RELAY_OFF 1

/**
 * Initialize relay GPIOs (output, default OFF).
 */
esp_err_t relays_init(void);

/**
 * Set a relay state.
 * @param id RELAY_PUMP / RELAY_LIGHT / RELAY_HEATER / RELAY_FAN
 * @param on true = ON, false = OFF
 */
void relay_set(int id, bool on);

/**
 * Get current relay state.
 */
bool relay_get(int id);

/**
 * Update all relays from global state flags.
 */
void relays_update(bool pump, bool light, bool heater, bool fan);
