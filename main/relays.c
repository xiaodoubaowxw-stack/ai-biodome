#include "relays.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "relays";

static const int relay_pins[RELAY_COUNT] = {8, 9, 10, 11};
static bool relay_states[RELAY_COUNT] = {false, false, false, false};

esp_err_t relays_init(void)
{
    for (int i = 0; i < RELAY_COUNT; i++) {
        gpio_config_t cfg = {
            .pin_bit_mask = (1ULL << relay_pins[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&cfg);
        gpio_set_level(relay_pins[i], RELAY_OFF);
        relay_states[i] = false;
    }
    ESP_LOGI(TAG, "Relays initialized (pump=8, light=9, heater=10, fan=11)");
    return ESP_OK;
}

void relay_set(int id, bool on)
{
    if (id < 0 || id >= RELAY_COUNT) return;
    relay_states[id] = on;
    gpio_set_level(relay_pins[id], on ? RELAY_ON : RELAY_OFF);
}

bool relay_get(int id)
{
    if (id < 0 || id >= RELAY_COUNT) return false;
    return relay_states[id];
}

void relays_update(bool pump, bool light, bool heater, bool fan)
{
    relay_set(RELAY_PUMP, pump);
    relay_set(RELAY_LIGHT, light);
    relay_set(RELAY_HEATER, heater);
    relay_set(RELAY_FAN, fan);
}
