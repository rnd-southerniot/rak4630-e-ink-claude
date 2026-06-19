#pragma once

#include "compat.h"

/*
 * Board package interface. Each board provides its own implementation under
 * src/board/<board>/ (selected by build_src_filter per env in platformio.ini).
 * This isolates the only genuinely MCU/module-specific code — the SX1262 radio
 * bring-up and the battery ADC — from the shared services/gates.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Bring up the on-board SX1262 (pins, SPI, TCXO, antenna switch). Called once by
 * lorawan_service_init() before lmh_init(). Returns ESP_OK on success. */
esp_err_t board_radio_init(void);

/* Read the battery voltage in volts (board-specific ADC + divider math).
 * Only called when APP_BATTERY_ADC_ENABLED; returns <0 if unavailable. */
float board_read_battery_volts(void);

#ifdef __cplusplus
}
#endif
