#pragma once
#include "esp_err.h"
#include <stdbool.h>

typedef struct {
    bool fan_en;
    char fan_start[6];  // "HH:MM"
    char fan_end[6];
    bool light_en;
    char light_start[6];
    char light_end[6];
} sched_config_t;

/**
 * Initialize scheduler (load config from NVS).
 */
esp_err_t scheduler_init(void);

/**
 * Run auto-control logic. Called periodically or on state change.
 */
void scheduler_run(void);

/**
 * Get const pointer to schedule config.
 */
const sched_config_t *scheduler_get_config(void);

/**
 * Get mutable pointer to schedule config (for WebSocket updates).
 */
sched_config_t *scheduler_get_config_mutable(void);

/**
 * SGP30 periodic read task (must be called every 1 second).
 */
void scheduler_sgp30_tick(void);
