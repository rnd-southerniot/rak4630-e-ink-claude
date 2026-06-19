/*
 * Board package — RAK3312 (ESP32-S3 + SX1262). Compiled only for the rak3312 env
 * (build_src_filter). Provides the SX1262 radio bring-up and battery ADC.
 *
 * SX1262 wiring from the RAKwireless RAK3312 datasheet (board_pins_rak3312.h).
 * The antenna-switch (GPIO4) and TCXO handling are the main bring-up unknowns —
 * tunable via the -D flags below; confirm empirically at gate 6 (join).
 */
#include <Arduino.h>
#include <string.h>
#include <LoRaWan-Arduino.h>

#include "board.h"
#include "board_pins.h"

static const char *TAG = "BOARD";

/* RAK3312 RF front-end config — adjust at gate-6 bring-up if join fails. */
#ifndef BOARD_RAK3312_USE_DIO2_ANTSW
#define BOARD_RAK3312_USE_DIO2_ANTSW 0      /* explicit ANT_SW pin, not DIO2 */
#endif
#ifndef BOARD_RAK3312_USE_TCXO
#define BOARD_RAK3312_USE_TCXO 1            /* RAK SX1262 modules use a TCXO */
#endif

esp_err_t board_radio_init(void)
{
    hw_config hwConfig;
    memset(&hwConfig, 0, sizeof(hwConfig));
    hwConfig.CHIP_TYPE          = SX1262_CHIP;
    hwConfig.PIN_LORA_RESET     = PIN_LORA_RESET_RAK3312;
    hwConfig.PIN_LORA_NSS       = PIN_LORA_NSS_RAK3312;
    hwConfig.PIN_LORA_SCLK      = PIN_LORA_SCLK_RAK3312;
    hwConfig.PIN_LORA_MISO      = PIN_LORA_MISO_RAK3312;
    hwConfig.PIN_LORA_MOSI      = PIN_LORA_MOSI_RAK3312;
    hwConfig.PIN_LORA_DIO_1     = PIN_LORA_DIO1_RAK3312;
    hwConfig.PIN_LORA_BUSY      = PIN_LORA_BUSY_RAK3312;
    /* Antenna switch: RAK3312 exposes an explicit ANT_SW GPIO (not DIO2). Drive
     * it via the library's RX-enable line with antenna power on. */
    hwConfig.RADIO_TXEN         = -1;
    hwConfig.RADIO_RXEN         = PIN_LORA_ANTSW_RAK3312;
    hwConfig.USE_RXEN_ANT_PWR   = true;
    hwConfig.USE_DIO2_ANT_SWITCH = (BOARD_RAK3312_USE_DIO2_ANTSW != 0);
    hwConfig.USE_DIO3_TCXO      = (BOARD_RAK3312_USE_TCXO != 0);
    hwConfig.USE_DIO3_ANT_SWITCH = false;
    hwConfig.USE_LDO            = false;

    uint32_t err = lora_hardware_init(hwConfig);
    if (err != 0) {
        ESP_LOGE(TAG, "lora_hardware_init_failed code=%lu", (unsigned long)err);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "radio_init_ok board=rak3312 chip=sx1262 nss=%d sclk=%d mosi=%d miso=%d busy=%d dio1=%d rst=%d antsw=%d tcxo=%d",
             PIN_LORA_NSS_RAK3312, PIN_LORA_SCLK_RAK3312, PIN_LORA_MOSI_RAK3312,
             PIN_LORA_MISO_RAK3312, PIN_LORA_BUSY_RAK3312, PIN_LORA_DIO1_RAK3312,
             PIN_LORA_RESET_RAK3312, PIN_LORA_ANTSW_RAK3312, BOARD_RAK3312_USE_TCXO);
    return ESP_OK;
}

float board_read_battery_volts(void)
{
    /* ESP32-S3 ADC: analogReadMilliVolts handles calibration/attenuation. VBAT
     * GPIO is unconfirmed on RAK3312 — returns <0 when not wired (PIN<0). */
    if (PIN_BATTERY_VBAT < 0) {
        return -1.0f;
    }
    uint32_t mv = 0;
    for (int i = 0; i < 8; i++) {
        mv += analogReadMilliVolts(PIN_BATTERY_VBAT);
    }
    float battery_v = ((float)(mv / 8u) / 1000.0f) * VBAT_DIVIDER_COMP;
    ESP_LOGI(TAG, "battery_adc mv=%u battery_v=%.3f", (unsigned)(mv / 8u), (double)battery_v);
    return battery_v;
}
