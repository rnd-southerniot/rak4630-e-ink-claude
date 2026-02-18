#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "app_types.h"
#include "esp_err.h"

esp_err_t display_service_init(void);
esp_err_t display_service_spi_bus_check(void);
esp_err_t display_service_render_hello_world(void);
esp_err_t display_service_render_white_clear(void);
bool display_service_should_refresh(
    const sensor_sample_t *last_rendered,
    const sensor_sample_t *current,
    uint64_t now_ms,
    uint64_t last_refresh_ms);
esp_err_t display_service_render(const sensor_sample_t *sample);
