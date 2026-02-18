# Gate 6 Example (`lorawan_join_uplink`)

## Purpose

Validate join + uplink control path with deterministic counters before reliability/live gates.

## Prerequisites

- `/Users/arif/rak4630-e-ink/firmware/.env` exists with:
  - `DEVEUI`
  - `APPKEY`
  - `JOINEUI=0000000000000000` (default)

## Commands (Prompt/CLI)

```bash
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 6
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

One-command shortcut:

```bash
/Users/arif/rak4630-e-ink/examples/gates/run_gate_6_lorawan_join_uplink.sh /dev/cu.usbmodem1101
```

## Expected Markers

- `APP: boot ... gate=6 ... name=lorawan_join_uplink`
- `LORAWAN: init_stub region=AS923-1 backend_active=1`
- `LORAWAN: join_start attempt=...`
- `LORAWAN: join_success attempts=...`
- `LORAWAN: uplink_stub_ok bytes=12`
- `APP: result=PASS gate=6 name=lorawan_join_uplink ...`
- `APP: handshake=STOP_AFTER_PASS gate=6 ...`

## PASS Rule

Gate passes only when joined state is reached and at least one uplink succeeds.
