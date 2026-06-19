#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_types.h"
#include "compat.h"

/*
 * Pluggable I2C sensor driver interface. Each sensor (SGP40, BMP280/BME280,
 * SHTC3, ...) provides one of these; the registry probes, initializes, and reads
 * all registered drivers, aggregating into a sensor_sample_t. Adding a sensor in
 * a WisBlock slot = implement a driver + register it (slots share one I2C bus).
 */
typedef struct sensor_driver_s {
    const char *name;   /* e.g. "SGP40", "BMP280", "SHTC3" */
    uint8_t addr;       /* primary I2C address */
    uint32_t fields;    /* SENSOR_FIELD_* this driver contributes */

    /* Optional post-probe init (calibration, algorithm setup). NULL = none. */
    esp_err_t (*init)(void);
    /* Human-readable identity (serial/chip-id) into buf; sets *ok. NULL = none. */
    esp_err_t (*identity)(char *buf, size_t buf_len, bool *ok);
    /* Read this driver's measurement into sample (OR its SENSOR_FIELD_* bits into
     * sample->present_mask). Must not clear other drivers' fields. */
    esp_err_t (*read)(sensor_sample_t *sample);

    bool present;       /* set by sensor_registry_probe_all() */
} sensor_driver_t;
