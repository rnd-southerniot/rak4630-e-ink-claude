#pragma once

#include <stdbool.h>
#include <stdint.h>

/*
 * Generic gate-runner harness — the reusable lifecycle + canonical log markers
 * of a "one gate per flash" bring-up system, decoupled from any specific gate
 * catalog. An application keeps its own gate enum / dispatch / per-gate state and
 * drives a gate_run_t through these helpers, passing the gate's id + name as
 * strings. Markers are stable (result=PASS/FAIL, handshake=STOP_AFTER_PASS,
 * halted_after_pass) so operator tooling and evidence parsing are unaffected.
 */

#define GATE_FW_PROGRESS_LOG_MS 2000ULL

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool pass;
    bool halted;
    uint32_t failures;
    uint64_t start_ms;
    uint64_t last_progress_log_ms;
    uint64_t last_idle_log_ms;
} gate_run_t;

/* Throttled progress line: "gate=<id> name=<name> <msg>". */
void gate_fw_progress(gate_run_t *run, uint64_t now_ms, const char *id, const char *name,
                      const char *fmt, ...);

/* Mark PASS exactly once: sets pass+halted, emits result=PASS + STOP_AFTER_PASS handshake. */
void gate_fw_mark_pass(gate_run_t *run, const char *id, const char *name, const char *fmt, ...);

/* Throttled FAIL line; increments run->failures. */
void gate_fw_mark_fail(gate_run_t *run, uint64_t now_ms, const char *id, const char *name,
                       const char *fmt, ...);

/* While halted, emit the throttled "halted_after_pass" heartbeat. */
void gate_fw_idle_tick(gate_run_t *run, uint64_t now_ms, const char *id, uint32_t idle_log_sec);

#ifdef __cplusplus
}
#endif
