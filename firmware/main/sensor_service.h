#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "app_types.h"
#include "esp_err.h"

typedef struct {
    bool sgp40_present;
    bool bmp280_present;
    bool sgp40_serial_ok;
    bool bmp280_chip_id_ok;
    uint8_t bmp280_chip_id;
    uint16_t sgp40_serial_words[3];
} sensor_identity_t;

esp_err_t sensor_service_init(void);
bool sensor_service_is_sgp40_present(void);
bool sensor_service_is_bmp280_present(void);
esp_err_t sensor_service_read_identity(sensor_identity_t *identity);
esp_err_t sensor_service_read_sgp40_identity(sensor_identity_t *identity);
esp_err_t sensor_service_read_bmp280_identity(sensor_identity_t *identity);
esp_err_t sensor_service_read_basic_raw(uint32_t *pressure_raw, uint32_t *temperature_raw);
esp_err_t sensor_service_read_bmp280(float *pressure_pa, float *temperature_c);
esp_err_t sensor_service_read_voc_only(float *voc_index);
float sensor_service_read_battery_v(void);
esp_err_t sensor_service_read(sensor_sample_t *sample);
