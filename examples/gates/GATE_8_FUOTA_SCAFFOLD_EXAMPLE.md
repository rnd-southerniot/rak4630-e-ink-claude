# Gate 8 Example (`fuota_scaffold`)

## Purpose

Validate FUOTA service hooks and rollback policy markers before live publish.

## Commands (Prompt/CLI)

```bash
/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 8
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh /dev/cu.usbmodem1101
```

One-command shortcut:

```bash
/Users/arif/rak4630-e-ink/examples/gates/run_gate_8_fuota_scaffold.sh /dev/cu.usbmodem1101
```

## Expected Markers

- `APP: boot ... gate=8 ... name=fuota_scaffold`
- `FUOTA: manifest=placeholder-v1 region=AS923-1 mode=emergency-only`
- `FUOTA: multicast_hook=ready fragment_hook=ready rollback_policy=hard_required`
- `FUOTA: version_payload=01 00 00 00 downgrade_block=1 max_downtime_min=30`
- `APP: result=PASS gate=8 name=fuota_scaffold ...`
- `APP: handshake=STOP_AFTER_PASS gate=8 ...`

## PASS Rule

Gate passes only when all FUOTA scaffolding markers are logged with rollback requirement asserted.
