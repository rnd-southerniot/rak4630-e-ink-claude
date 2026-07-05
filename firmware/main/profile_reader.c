/* profile_reader.c — profile-driven I2C sensor read via compiled per-sensor drivers (see .h). */
#include "profile_reader.h"

#include "esp_log.h"
#include "i2c_bus.h"
#include "sensor_service.h"

static const char *TAG = "profrd";

esp_err_t profile_reader_init(void)
{
    /* Bring up the I2C bus first (the gate framework normally does this; the LoRa field path does
     * not). Then sensor_service_init — a missing NON-target sensor (e.g. no SGP40 on the bench) is a
     * warning, not fatal: the per-sensor read reports its own failure. */
    const esp_err_t bus = i2c_bus_init();
    if (bus != ESP_OK) {
        ESP_LOGE(TAG, "i2c_bus_init failed: %s", esp_err_to_name(bus));
        return bus;
    }
    const esp_err_t sens = sensor_service_init();
    if (sens != ESP_OK) {
        ESP_LOGW(TAG, "sensor_service_init: %s (continuing; a missing non-target sensor is OK)",
                 esp_err_to_name(sens));
    }
    return ESP_OK;
}

esp_err_t profile_read(const device_profile_t *p, float *values, size_t n_values)
{
    if (p == NULL || values == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (p->bus_kind != DP_BUS_I2C) {
        ESP_LOGW(TAG, "profile bus_kind=%u not I2C — unsupported here", (unsigned)p->bus_kind);
        return ESP_ERR_NOT_SUPPORTED;
    }
    for (size_t i = 0; i < n_values; ++i) {
        values[i] = 0.0f;
    }

    switch (p->sensor_type) {
    case DP_SENSOR_BME280: {
        /* Bosch temp + pressure via the compiled compensation driver. Measurand order:
         * 0 = temperature (C), 1 = pressure (hPa). (Humidity would be a 3rd once a BME280 —
         * not BMP280 — driver read is added.) */
        if (n_values < 2) {
            return ESP_ERR_INVALID_SIZE;
        }
        float pressure_pa = 0.0f, temperature_c = 0.0f;
        const esp_err_t err = sensor_service_read_bmp280(&pressure_pa, &temperature_c);
        if (err != ESP_OK) {
            return err;
        }
        values[0] = temperature_c;
        values[1] = pressure_pa / 100.0f;
        return ESP_OK;
    }
    default:
        ESP_LOGW(TAG, "sensor_type=%u has no compiled reader", (unsigned)p->sensor_type);
        return ESP_ERR_NOT_SUPPORTED;
    }
}
