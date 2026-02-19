# Gate 7 Example (`reliability_buffer`)

## Purpose

Validate pre-join payload buffering and post-join flush behavior with deterministic counters.

## Prerequisites

- `firmware/.env` exists with:
  - `DEVEUI`
  - `APPKEY`
  - `JOINEUI=0000000000000000` (default)

## Commands (Prompt/CLI)

```bash
examples/gates/set_gate_new.sh 7
examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

One-command shortcut:

```bash
examples/gates/run_gate_7_reliability_buffer.sh /dev/cu.usbmodem1101
```

## Expected Markers

- `APP: boot ... gate=7 ... name=reliability_buffer`
- `LORAWAN: backend_active_set value=0`
- `APP: gate=7 buffer_store bytes=12`
- `LORAWAN: backend_active_set value=1`
- `LORAWAN: join_success attempts=...`
- `APP: gate=7 buffer_flush_ok flushed_count=1`
- `APP: result=PASS gate=7 name=reliability_buffer ...`
- `APP: handshake=STOP_AFTER_PASS gate=7 ...`

## PASS Rule

Gate passes only when at least one payload is buffered while not joined and later flushed after join.
