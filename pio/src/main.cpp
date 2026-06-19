/*
 * RAK4630 (nRF52840) WisBlock Environmental Node — Arduino/PlatformIO entry point.
 *
 * Ported from ESP-IDF firmware. Gate-driven architecture preserved:
 * - Exactly one gate runs per flash cycle (set APP_GATE in platformio.ini)
 * - 200ms tick loop polls app_gate_tick()
 * - On PASS, firmware halts and logs result=PASS
 *
 * To change gate: edit -DAPP_GATE=<N> in platformio.ini build_flags
 */

#include <Arduino.h>

#include "app_gate.h"
#include "heartbeat.h"
#include "hal_gpio.h"
#include "board_pins.h"

static app_gate_ctx_t gate_ctx;

void setup()
{
    /* Initialize USB CDC serial */
    Serial.begin(115200);

    /* Wait for serial on nRF52840 USB CDC (timeout after 3s for headless operation) */
    uint32_t start = millis();
    while (!Serial && (millis() - start) < 3000) {
        delay(10);
    }

    Serial.println();
    Serial.println("[I] APP: === RAK4630 WisBlock Environmental Node ===");
    Serial.println("[I] APP: Platform: nRF52840 + SX1262 (Arduino/PlatformIO)");
    Serial.printf("[I] APP: Build: %s %s\r\n", __DATE__, __TIME__);
    Serial.printf("[I] APP: APP_GATE=%d\r\n", APP_GATE);

    esp_err_t err = app_gate_init(&gate_ctx);
    if (err != ESP_OK) {
        Serial.printf("[E] APP: gate_init_failed err=%s\r\n", esp_err_to_name(err));
        /* Fatal — halt with error LED blink */
        hal_gpio_mode(PIN_LED_GREEN, HAL_GPIO_OUTPUT);
        while (1) {
            hal_gpio_write(PIN_LED_GREEN, true);
            delay(100);
            hal_gpio_write(PIN_LED_GREEN, false);
            delay(100);
        }
    }

    Serial.printf("[I] APP: gate_runner_started gate=%d name=%s\r\n",
                  (int)gate_ctx.selected, app_gate_name(gate_ctx.selected));
}

void loop()
{
    uint64_t now_ms = (uint64_t)millis();
    app_gate_tick(&gate_ctx, now_ms);

    /* Also update heartbeat if gate 1 is running (cooperative scheduling) */
    if (gate_ctx.selected == APP_GATE_1_HEARTBEAT) {
        heartbeat_update();
    }

    delay(APP_GATE_TICK_MS);
}
