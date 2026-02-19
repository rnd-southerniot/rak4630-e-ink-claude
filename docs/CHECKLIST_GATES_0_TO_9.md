# CHECKLIST Gates 0..9 + 2.1 (Canonical)

## Preflight

- [ ] Pin mapping reviewed in `docs/11-pin-mapping-rak3312-rak19007.md`
- [ ] Canonical gate mode selected (`CONFIG_APP_GATE_ID_SCHEME_NEW=y`) unless legacy test intentionally enabled
- [ ] Serial port confirmed
- [ ] `.env` present for Gate 6/7/9 (with `DEVEUI`, `APPKEY`, default `JOINEUI`)

## Gate Pass Checklist

- [ ] Gate 0 PASS (`env`)
- [x] Gate 1 PASS (`heartbeat`)
- [x] Gate 2 PASS (`display_smoke`) with visible `black -> red -> final_text_card` (operator confirmed visible text on 2026-02-17)
- [x] Gate 2.1 PASS (`i2c_smoke`) with selected mode `1` (SGP40 only, rerun on 2026-02-17)
- [x] Gate 3 PASS (`i2c_presence`) with selected mode `1` (SGP40 only, rerun on 2026-02-17)
- [x] Gate 4 PASS (`sensor_pipeline`) with selected mode `1` (SGP40-only real read, rerun on 2026-02-17)
- [x] Gate 5 PASS (`payload_v1`) and host payload test PASS (on-device rerun PASS on 2026-02-17)
- [x] Gate 6 PASS (`lorawan_join_uplink`) (on-device rerun PASS on 2026-02-17)
- [x] Gate 7 PASS (`reliability_buffer`) (on-device rerun PASS on 2026-02-17, post Gate 6 example/doc sync)
- [x] Gate 8 PASS (`fuota_scaffold`) (on-device rerun PASS on 2026-02-17, post Gate 7 example/doc sync)
- [x] Gate 9 PASS (`live_publish`) with visible display data (mode `1`, on-device rerun PASS on 2026-02-17, post Gate 8 example/doc sync)

## Mandatory Artifacts

- [ ] `docs/GATE_EXECUTION_LOG.md` updated with evidence lines for each gate
- [ ] Legacy mapping log captured if legacy mode used
- [ ] Build matrix evidence captured (Gate 2 + Gate 2.1 + Gate 9, panel profiles as required)

## Build Matrix Verification

- [x] `Gate=2`, panel `SSD1680_250X122` build PASS
- [x] `Gate=21` (Gate 2.1), I2C smoke build PASS
- [ ] `Gate=9`, panel `SSD1680_250X122` build PASS
- [ ] `Gate=2`, panel `SSD1680_212X104` build PASS
- [ ] `Gate=9`, panel `SSD1680_212X104` build PASS
