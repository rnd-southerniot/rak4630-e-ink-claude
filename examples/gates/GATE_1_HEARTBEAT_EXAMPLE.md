# Gate 1 Example (`heartbeat`)

## Purpose

Validate heartbeat LED pin mapping and deterministic PASS handshake.

## Pin Precheck

From pin mapping source-of-truth:

- Primary heartbeat LED: `GPIO46` (`LED_GREEN`)
- Alternate heartbeat LED: `GPIO45` (`LED_BLUE`)

Reference:
- `/Users/arif/rak4630-e-ink/docs/11-pin-mapping-rak3312-rak19007.md`

## Commands (Prompt/CLI)

```bash
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 1
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

One-command shortcut:

```bash
/Users/arif/rak4630-e-ink/examples/gates/run_gate_1_heartbeat.sh /dev/cu.usbmodem1101
```

## Expected Markers

- `APP: gate=1 name=heartbeat ...`
- `APP: heartbeat_started gpio=46`
- `APP: result=PASS gate=1 name=heartbeat ...`
- `APP: handshake=STOP_AFTER_PASS gate=1 action=change_CONFIG_APP_GATE_and_reflash`

## PASS Rule

Gate passes only if:
- firmware logs the PASS marker, and
- operator confirms visible LED blinking.
