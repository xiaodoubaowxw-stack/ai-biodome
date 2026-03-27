#pragma once
#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#define SHT40_ADDR   0x44
#define BH1750_ADDR  0x23
#define SGP30_ADDR   0x58

#define I2C_SDA_PIN  4
#define I2C_SCL_PIN  5
#define SOIL_ADC_PIN 6

#define SOIL_AIR_VAL   4095
#define SOIL_WATER_VAL 1200

typedef struct {
    float    temperature;
    float    humidity;
    float    lux;
    int      soil_percent;
    uint16_t eco2;
    uint16_t tvoc;
} sensor_data_t;

/**
 * Initialize I2C bus and all sensors.
 */
esp_err_t sensors_init(void);

/**
 * Read SHT40 temperature and humidity.
 */
esp_err_t sensor_read_sht40(float *temp, float *hum);

/**
 * Read BH1750 light level in lux.
 */
esp_err_t sensor_read_bh1750(float *lux);

/**
 * Read SGP30 eCO2 and TVOC. Should be called once per second.
 */
esp_err_t sensor_read_sgp30(uint16_t *eco2, uint16_t *tvoc);

/**
 * Read soil moisture ADC and return percentage (0-100).
 */
int sensor_read_soil(void);

/**
 * Read all sensors at once and fill sensor_data_t.
 */
esp_err_t sensors_read_all(sensor_data_t *data);
