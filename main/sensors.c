#include "sensors.h"
#include "driver/i2c_master.h"
#include "driver/adc.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "sensors";

static i2c_master_bus_handle_t s_i2c_bus = NULL;
static i2c_master_dev_handle_t s_sht40_dev = NULL;
static i2c_master_dev_handle_t s_bh1750_dev = NULL;
static i2c_master_dev_handle_t s_sgp30_dev = NULL;
static adc_oneshot_unit_handle_t s_adc_handle = NULL;

/* ---- I2C init ---- */
esp_err_t sensors_init(void)
{
    /* I2C bus */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_PIN,
        .scl_io_num = I2C_SCL_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* SHT40 */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHT40_ADDR,
        .scl_speed_hz = 100000,
    };
    i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &s_sht40_dev);

    /* BH1750 */
    dev_cfg.device_address = BH1750_ADDR;
    i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &s_bh1750_dev);

    /* SGP30 */
    dev_cfg.device_address = SGP30_ADDR;
    i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &s_sgp30_dev);

    /* Start BH1750 continuous high-res mode */
    uint8_t bh_cmd = 0x10;
    i2c_master_transmit(s_bh1750_dev, &bh_cmd, 1, 100);

    /* Init SGP30 */
    uint8_t sgp_init[] = {0x20, 0x03};  // IAQ init
    i2c_master_transmit(s_sgp30_dev, sgp_init, 2, 100);
    vTaskDelay(pdMS_TO_TICKS(10));  // SGP30 needs 10ms after init

    /* ADC for soil moisture */
    adc_oneshot_unit_init_cfg_t adc_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ret = adc_oneshot_new_unit(&adc_cfg, &s_adc_handle);
    if (ret == ESP_OK) {
        adc_oneshot_chan_cfg_t chan_cfg = {
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12,
        };
        adc_oneshot_config_channel(s_adc_handle, ADC_CHANNEL_5, &chan_cfg);  // GPIO6 = ADC1_CH5
    }

    ESP_LOGI(TAG, "Sensors initialized (I2C SDA=%d SCL=%d)", I2C_SDA_PIN, I2C_SCL_PIN);
    return ESP_OK;
}

/* ---- SHT40 ---- */
esp_err_t sensor_read_sht40(float *temp, float *hum)
{
    uint8_t cmd = 0xFD;  // high precision, no heater
    uint8_t buf[6] = {0};

    esp_err_t ret = i2c_master_transmit(s_sht40_dev, &cmd, 1, 100);
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(10));

    ret = i2c_master_receive(s_sht40_dev, buf, 6, 100);
    if (ret != ESP_OK) return ret;

    uint16_t t_ticks = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t h_ticks = ((uint16_t)buf[3] << 8) | buf[4];

    *temp = -45.0f + (175.0f * t_ticks / 65535.0f);
    *hum  = -6.0f + (125.0f * h_ticks / 65535.0f);

    return ESP_OK;
}

/* ---- BH1750 ---- */
esp_err_t sensor_read_bh1750(float *lux)
{
    uint8_t buf[2] = {0};
    esp_err_t ret = i2c_master_receive(s_bh1750_dev, buf, 2, 100);
    if (ret != ESP_OK) return ret;

    uint16_t raw = ((uint16_t)buf[0] << 8) | buf[1];
    *lux = raw / 1.2f;
    return ESP_OK;
}

/* ---- SGP30 ---- */
esp_err_t sensor_read_sgp30(uint16_t *eco2, uint16_t *tvoc)
{
    uint8_t cmd[] = {0x20, 0x08};  // IAQ measure
    uint8_t buf[6] = {0};

    esp_err_t ret = i2c_master_transmit(s_sgp30_dev, cmd, 2, 100);
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(12));

    ret = i2c_master_receive(s_sgp30_dev, buf, 6, 100);
    if (ret != ESP_OK) return ret;

    *eco2 = ((uint16_t)buf[0] << 8) | buf[1];
    *tvoc = ((uint16_t)buf[3] << 8) | buf[4];

    return ESP_OK;
}

/* ---- Soil moisture ---- */
int sensor_read_soil(void)
{
    if (!s_adc_handle) return 0;
    int raw = 0;
    adc_oneshot_read(s_adc_handle, ADC_CHANNEL_5, &raw);

    /* Map: air=4095 (0%), water=1200 (100%) */
    int pct = (int)((float)(SOIL_AIR_VAL - raw) / (SOIL_AIR_VAL - SOIL_WATER_VAL) * 100.0f);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

/* ---- Read all ---- */
esp_err_t sensors_read_all(sensor_data_t *data)
{
    memset(data, 0, sizeof(*data));

    if (sensor_read_sht40(&data->temperature, &data->humidity) != ESP_OK) {
        ESP_LOGW(TAG, "SHT40 read failed");
    }
    if (sensor_read_bh1750(&data->lux) != ESP_OK) {
        ESP_LOGW(TAG, "BH1750 read failed");
    }
    data->soil_percent = sensor_read_soil();

    return ESP_OK;
}
