/*
 * Sensor driver registry — probes, initializes and reads a set of pluggable
 * I2C sensor drivers on the shared WisBlock bus, aggregating into one sample.
 */
#include <Arduino.h>
#include <string.h>

#include "sensor_registry.h"
#include "i2c_bus.h"

static const char *TAG = "SENSOR";

#ifndef SENSOR_REGISTRY_MAX
#define SENSOR_REGISTRY_MAX 8
#endif

static sensor_driver_t *s_drivers[SENSOR_REGISTRY_MAX];
static size_t s_count;

void sensor_registry_reset(void)
{
    memset(s_drivers, 0, sizeof(s_drivers));
    s_count = 0;
}

esp_err_t sensor_registry_add(sensor_driver_t *driver)
{
    if (driver == NULL) return ESP_ERR_INVALID_ARG;
    if (s_count >= SENSOR_REGISTRY_MAX) return ESP_ERR_NO_MEM;
    driver->present = false;
    s_drivers[s_count++] = driver;
    return ESP_OK;
}

size_t sensor_registry_count(void) { return s_count; }

sensor_driver_t *sensor_registry_get(size_t index)
{
    return (index < s_count) ? s_drivers[index] : NULL;
}

sensor_driver_t *sensor_registry_find(const char *name)
{
    if (name == NULL) return NULL;
    for (size_t i = 0; i < s_count; i++) {
        if (strcmp(s_drivers[i]->name, name) == 0) return s_drivers[i];
    }
    return NULL;
}

esp_err_t sensor_registry_probe_all(void)
{
    for (size_t i = 0; i < s_count; i++) {
        sensor_driver_t *d = s_drivers[i];
        bool found = false;
        esp_err_t err = i2c_bus_probe(d->addr, &found);
        d->present = (err == ESP_OK) && found;
        ESP_LOGI(TAG, "registry_probe name=%s addr=0x%02X present=%d", d->name, d->addr, d->present);
        if (d->present && d->init != NULL) {
            esp_err_t ierr = d->init();
            if (ierr != ESP_OK) {
                ESP_LOGW(TAG, "registry_init_failed name=%s err=%s", d->name, esp_err_to_name(ierr));
            }
        }
    }
    return ESP_OK;
}

size_t sensor_registry_present_count(void)
{
    size_t n = 0;
    for (size_t i = 0; i < s_count; i++) {
        if (s_drivers[i]->present) n++;
    }
    return n;
}

esp_err_t sensor_registry_read_all(sensor_sample_t *sample)
{
    if (sample == NULL) return ESP_ERR_INVALID_ARG;

    size_t present = 0, ok = 0;
    for (size_t i = 0; i < s_count; i++) {
        sensor_driver_t *d = s_drivers[i];
        if (!d->present || d->read == NULL) continue;
        present++;
        esp_err_t err = d->read(sample);
        if (err == ESP_OK) {
            ok++;
        } else {
            ESP_LOGW(TAG, "registry_read_fail name=%s err=%s", d->name, esp_err_to_name(err));
        }
    }

    sample->timestamp_ms = (uint64_t)millis();
    /* Valid when every present driver read OK and at least one was present. */
    sample->valid = (present > 0) && (ok == present);
    return sample->valid ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}
