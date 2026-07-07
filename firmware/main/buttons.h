/*
 * buttons.h — RAK14000 3-button module (S1/S2/S3 on GPIO41/38/39). Active-low, FALLING-edge
 * interrupt inputs with ISR-level debounce; presses land in a small queue drained by buttons_take().
 * Functions are TBD — a press currently just logs + forces a display refresh (see app_gate).
 */
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Configure the 3 buttons as debounced FALLING-edge interrupt inputs. Call once at boot. */
esp_err_t buttons_init(void);

/* Non-blocking: if a debounced press is pending, set *btn to 1/2/3 (S1/S2/S3) and return true. */
bool buttons_take(uint8_t *btn);

#ifdef __cplusplus
}
#endif

#endif /* BUTTONS_H */
