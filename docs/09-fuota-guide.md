# FUOTA Guide (ESP-IDF Device + ChirpStack v4)

This guide is tailored for:
- Core: `RAK3312` (`ESP32-S3 + SX1262`)
- Gateway: `RAK7266` (`Semtech UDP packet forwarder`)
- Region: `AS923-1`
- Country: `Bangladesh`
- Server: `ChirpStack v4` on `Docker`
- Policy: rollback is mandatory
- Cadence: `emergency-only`
- Max downtime per campaign: `30 minutes`

## 1) Pre-checks

1. Verify normal OTAA uplink works every 5 minutes before FUOTA work.
2. Verify gateway is stable in ChirpStack (`last seen` updating).
3. Confirm firmware image size fits OTA partition budget.

## 2) Device Firmware Requirements

1. Enable dual-app OTA partition table (factory + ota_0 + ota_1 recommended).
2. Implement secure update write path and image validation.
3. Add boot health-check logic:
   - On successful startup health window, call `esp_ota_mark_app_valid_cancel_rollback()`.
   - On critical startup failure, call `esp_ota_mark_app_invalid_rollback_and_reboot()`.
4. Expose current firmware version over uplink or diagnostic command.

## 3) ChirpStack v4 FUOTA Setup

1. Deploy/enable FUOTA capability per ChirpStack docs.
2. Create FUOTA deployment profile for `AS923-1`.
3. Create multicast/campaign group using only canary devices first.
4. Upload firmware artifact and set fragmentation/redundancy parameters.

Recommended initial campaign values:
- Small canary batch (1-3 nodes)
- Conservative fragment redundancy (for example 10-20%)
- Strict campaign timeout at 30 minutes with auto-fail on non-completion

## 4) Rollout Procedure

1. Canary:
   - 1 node, observe full update and post-boot health checks.
2. Pilot:
   - 5-10 nodes, monitor failure ratio and join/uplink recovery.
3. Production:
   - Expand only if canary/pilot pass rollback and stability thresholds.

## 5) Rollback Test (Mandatory)

Run one forced-failure campaign in lab:
1. Push an image that intentionally fails a startup health check.
2. Verify device reboots to previous known-good partition automatically.
3. Verify telemetry indicates rollback event.

Pass criteria:
- No manual intervention required to recover.
- Device returns to 5-minute normal telemetry cycle.

## 6) Operational Monitoring

Track per campaign:
- success rate
- timeout rate
- rollback count
- mean recovery time

Stop condition:
- Any rollback-rate spike above agreed threshold.
- Any campaign exceeding the 30-minute downtime budget.

## 7) Safety Rules

- Never run first-time FUOTA directly on production fleet.
- Keep previous firmware artifact and manifest available.
- Freeze application payload schema changes during FUOTA campaigns.

## References

- ChirpStack FUOTA guide: https://www.chirpstack.io/docs/chirpstack/use/fuota.html
- ChirpStack docs: https://www.chirpstack.io/docs/chirpstack/
- ESP-IDF OTA API reference: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/ota.html
