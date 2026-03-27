#pragma once
#include "esp_err.h"
#include <stdint.h>

#define HISTORY_SIZE 60

typedef struct {
    float    temp[HISTORY_SIZE];
    float    hum[HISTORY_SIZE];
    float    lux[HISTORY_SIZE];
    int      soil[HISTORY_SIZE];
    uint16_t eco2[HISTORY_SIZE];
    uint16_t tvoc[HISTORY_SIZE];
    char     time[HISTORY_SIZE][12];  // "HH:MM:SS" or "12345s"
    int      head;
    int      count;
} history_t;

/**
 * Initialize history buffer.
 */
void history_init(void);

/**
 * Record a data point.
 */
void history_record(float temp, float hum, float lux, int soil,
                    uint16_t eco2, uint16_t tvoc);

/**
 * Get pointer to internal history data.
 */
const history_t *history_get(void);

/**
 * Build cJSON array representation for WebSocket transmission.
 * Caller must free the returned cJSON object.
 */
void *history_build_json(void);
