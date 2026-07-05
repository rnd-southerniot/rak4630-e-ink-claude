/*
 * profile_reader.h — generic profile-driven sensor read (ADR-006, I2C flavour).
 *
 * For a DP_BUS_I2C profile the read is done by a COMPILED per-sensor driver selected by
 * profile->sensor_type (compensation-heavy sensors like BME280 can't be expressed as a raw register
 * map). The driver fills values[] in the profile's measurand order; dp_encode_payload then turns
 * values[] into the ADR-005 uplink. Adding a sensor = one case here + one compiled driver.
 */
#ifndef PROFILE_READER_H
#define PROFILE_READER_H

#include <stddef.h>

#include "device_profile.h"
#include "esp_err.h"

/* Bring up the sensor bus (I2C + sensor_service) once before profile_read(). */
esp_err_t profile_reader_init(void);

/*
 * Read the sensor described by `p` into values[0..n_values-1] (engineering units, in the profile's
 * measurand order). Returns ESP_OK on a good read, an error otherwise (caller flags STALE).
 * Only DP_BUS_I2C profiles are supported here.
 */
esp_err_t profile_read(const device_profile_t *p, float *values, size_t n_values);

#endif /* PROFILE_READER_H */
