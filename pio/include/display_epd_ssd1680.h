#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "compat.h"

typedef struct {
    int width;
    int height;
    int spi_mhz;
    int pin_mosi;
    int pin_miso;
    int pin_sclk;
    int pin_cs;
    int pin_dc;
    int pin_rst;
    int pin_busy;
    int pin_pwr;
    int xram_offset;
    bool partial_updates;
    bool busy_active_high;
    bool pwr_active_high;
    bool pwr_input_pullup;
} display_epd_ssd1680_config_t;

typedef struct {
    bool reset_busy_pulse_seen;
    bool refresh_busy_pulse_seen;
    int busy_level_snapshot;
    uint32_t spi_tx_calls;
    uint32_t spi_tx_bytes;
} display_epd_ssd1680_diag_t;

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t display_epd_ssd1680_init(const display_epd_ssd1680_config_t *cfg);
esp_err_t display_epd_ssd1680_draw_frame(const uint8_t *framebuffer, size_t len);
esp_err_t display_epd_ssd1680_draw_frame_dual(const uint8_t *bw_framebuffer, const uint8_t *red_framebuffer, size_t len);
void display_epd_ssd1680_get_diag(display_epd_ssd1680_diag_t *diag);
int display_epd_ssd1680_width(void);
int display_epd_ssd1680_height(void);

#ifdef __cplusplus
}
#endif
