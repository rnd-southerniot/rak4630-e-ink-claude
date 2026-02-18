# GATE TRANSITION HANDSHAKE

## Rules

1. Run one gate at a time.
2. On PASS, firmware must halt progression and keep heartbeat/idle logs only.
3. Operator must explicitly select next gate and reflash.
4. Never skip failed gate.

Expected PASS/hold markers:

- `APP: result=PASS gate=<N> name=<name> ...`
- `APP: handshake=STOP_AFTER_PASS gate=<N> action=change_CONFIG_APP_GATE_and_reflash`

## Canonical vs Legacy IDs

- Canonical mode (default): `CONFIG_APP_GATE_ID_SCHEME_NEW=y`, use `CONFIG_APP_GATE=0..9`; use `CONFIG_APP_GATE=21` for Gate 2.1.
- Legacy mode: `CONFIG_APP_GATE_ID_SCHEME_LEGACY=y`, set `CONFIG_APP_GATE_LEGACY=0..8`.
- Legacy translation log must appear:
  - `APP: gate_id_scheme=legacy input=<old> resolved=<new>`

Legacy mapping:

- `0->0`, `1->1`, `2->3`, `3->4`, `4->5`, `5->6`, `6->7`, `7->8`, `8->9`

## Mandatory Prompts

- Gate 1: no sensor prompt; require operator LED blink confirmation after firmware PASS.
- Gate 2.1: ask connected I2C devices (`1|2|3`) and set `CONFIG_APP_GATE2P1_EXPECTED_DEVICES`.
- Gate 3: ask connected I2C devices (`1|2|3`).
- Gate 4: ask selected sensor pipeline mode (`1|2|3`).
- Gate 6, 7, 9: ask for `DEVEUI` and `APPKEY`; use `JOINEUI=0000000000000000` default.
- After Gate 8 PASS, before Gate 9: ask selected device mode (`1|2|3`) and set `CONFIG_APP_GATE9_EXPECTED_DEVICES`.

## Preflight Before Every Gate

1. Check `/Users/arif/rak4630-e-ink/docs/11-pin-mapping-rak3312-rak19007.md`.
2. Confirm only target gate hardware pins are in scope.
3. Build, flash, monitor.
4. Record PASS/FAIL evidence.
