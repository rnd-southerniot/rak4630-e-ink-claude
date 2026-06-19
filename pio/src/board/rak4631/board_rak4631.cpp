/*
 * Board package — RAK4630 (nRF52840 + SX1262). Compiled only for the rak4631 env
 * (build_src_filter). Provides the SX1262 radio bring-up and battery ADC.
 */
#include <Arduino.h>
#include <LoRaWan-Arduino.h>

#include "board.h"
#include "board_pins.h"

static const char *TAG = "BOARD";

/*
 * SX126x-Arduino declares SPI_LORA as `extern` and only defines it for variants
 * that set _VARIANT_RAK4630_. Our variant identifies as _VARIANT_RAK4631_, so we
 * provide it here matching the library's RAK4630 LoRa-SPI map (NRF_SPIM2,
 * MISO=P1.13/45, SCK=P1.11/43, MOSI=P1.12/44). Guarded against double-definition.
 */
#if defined(NRF52_SERIES) && !defined(_VARIANT_RAK4630_)
SPIClass SPI_LORA(NRF_SPIM2, 45, 43, 44);
#endif

esp_err_t board_radio_init(void)
{
    /* RAK4630 has a fixed on-module SX1262 wiring; the library's helper sets the
     * pins, TCXO (DIO3) and DIO2 antenna switch for us. */
    uint32_t err = lora_rak4630_init();
    if (err != 0) {
        ESP_LOGE(TAG, "lora_rak4630_init_failed code=%lu", (unsigned long)err);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "radio_init_ok board=rak4631 chip=sx1262");
    return ESP_OK;
}

float board_read_battery_volts(void)
{
    /* nRF52840 VBAT on WB_A0 (P0.05/AIN3): averaged 12-bit read at the internal
     * 3.0 V reference, then divider compensation. (RAKwireless battery example.) */
    analogReference(AR_INTERNAL_3_0);
    analogReadResolution(12);

    uint32_t acc = 0;
    for (int i = 0; i < 8; i++) {
        acc += (uint32_t)analogRead(PIN_BATTERY_VBAT);
    }
    analogReference(AR_DEFAULT);

    uint16_t raw = (uint16_t)(acc / 8u);
    float battery_v = ((float)raw * 3.0f / 4096.0f) * VBAT_DIVIDER_COMP;
    ESP_LOGI(TAG, "battery_adc raw=%u battery_v=%.3f", (unsigned)raw, (double)battery_v);
    return battery_v;
}
