#pragma once

#include <stddef.h>

#include "sensor_driver.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Registry of pluggable sensor drivers (shared I2C bus). */
void sensor_registry_reset(void);
esp_err_t sensor_registry_add(sensor_driver_t *driver);
size_t sensor_registry_count(void);
sensor_driver_t *sensor_registry_get(size_t index);
sensor_driver_t *sensor_registry_find(const char *name);

/* Probe each driver on the bus (sets driver->present) and init the present ones. */
esp_err_t sensor_registry_probe_all(void);
size_t sensor_registry_present_count(void);

/* Read every present driver, aggregating into sample (sets present_mask + valid). */
esp_err_t sensor_registry_read_all(sensor_sample_t *sample);

#ifdef __cplusplus
}
#endif
