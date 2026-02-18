#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct {
    uint32_t found_count;
    bool sgp40_found;
    bool bmp280_found;
} i2c_scan_summary_t;

esp_err_t i2c_bus_init(void);
esp_err_t i2c_bus_probe(uint8_t addr, bool *found);
esp_err_t i2c_bus_write(uint8_t addr, const uint8_t *data, size_t len);
esp_err_t i2c_bus_read(uint8_t addr, uint8_t *data, size_t len);
esp_err_t i2c_bus_write_read(uint8_t addr, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len);
uint32_t i2c_bus_scan(i2c_scan_summary_t *summary);
