/*
 * Generic gate-runner harness implementation. Emits the canonical markers under
 * the "APP" tag, byte-identical to the original inline gate runner.
 */
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>

#include "gate_framework.h"
#include "compat.h"

static const char *TAG = "APP";

void gate_fw_progress(gate_run_t *run, uint64_t now_ms, const char *id, const char *name,
                      const char *fmt, ...)
{
    if (run == NULL) return;
    if (now_ms - run->last_progress_log_ms < GATE_FW_PROGRESS_LOG_MS) return;

    char line[192] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    ESP_LOGI(TAG, "gate=%s name=%s %s", id, name, line);
    run->last_progress_log_ms = now_ms;
}

void gate_fw_mark_pass(gate_run_t *run, const char *id, const char *name, const char *fmt, ...)
{
    if (run == NULL || run->halted) return;

    char line[192] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    run->pass = true;
    run->halted = true;
    ESP_LOGI(TAG, "result=PASS gate=%s name=%s %s", id, name, line);
    ESP_LOGI(TAG, "handshake=STOP_AFTER_PASS gate=%s action=change_APP_GATE_and_reflash", id);
}

void gate_fw_mark_fail(gate_run_t *run, uint64_t now_ms, const char *id, const char *name,
                       const char *fmt, ...)
{
    if (run == NULL) return;
    if (now_ms - run->last_progress_log_ms < GATE_FW_PROGRESS_LOG_MS) return;

    char line[192] = {0};
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    run->failures++;
    ESP_LOGW(TAG, "result=FAIL gate=%s name=%s failures=%lu %s",
             id, name, (unsigned long)run->failures, line);
    run->last_progress_log_ms = now_ms;
}

void gate_fw_idle_tick(gate_run_t *run, uint64_t now_ms, const char *id, uint32_t idle_log_sec)
{
    if (run == NULL) return;
    if ((now_ms - run->last_idle_log_ms) >= ((uint64_t)idle_log_sec * 1000ULL)) {
        ESP_LOGI(TAG, "gate=%s halted_after_pass=1 next_action=set_APP_GATE_and_reflash", id);
        run->last_idle_log_ms = now_ms;
    }
}
