# LoRaWAN (OTAA, AS923-1) + ChirpStack v4 + FUOTA Plan

## 1) OTAA Integration Plan

## Device-side

- Provision and validate per-device:
  - `DevEUI`
  - `JoinEUI` (AppEUI)
  - `AppKey`
- Implement join/uplink state machine:
  - idle -> join-request -> joined -> uplink -> retry/backoff
- Persist session context when supported by selected stack.
- Schedule sensor sampling and uplink every `5 minutes`.

## Gateway and Network-side

- Gateway model: `RAK7266`
- Packet forwarder: `Semtech UDP packet forwarder`
- Deployment country: `Bangladesh`
- ChirpStack component requirement: enable Semtech UDP support in Gateway Bridge path.
- Register gateway EUI and verify `last seen` updates before device join testing.

## ChirpStack v4-side

- Create tenant, application, and device profile.
- Deploy ChirpStack v4 on `Docker`.
- Use region profile `AS923-1` consistently.
- Register each device with per-device OTAA credentials.
- Configure payload decoder and event monitoring.

## AS923-1 Notes

- Device region, gateway channel plan, and ChirpStack profile must all be `AS923-1`.
- Mismatch at any layer causes join/uplink failure.

## 2) Payload Design

Recommended compact binary uplink:
- `schema_version` (uint8)
- `voc_index` (uint16)
- `pressure_pa` (uint32)
- `temperature_centi_c` (int16)
- `battery_mv` (uint16)
- `status_flags` (uint8)

## 3) Reliability and Observability

- Maintain counters:
  - join attempts/failures
  - uplink successes/failures
  - sensor read failures
  - display refresh failures
- Log last error reason in retained memory where possible.
- Add CLI command to print LoRaWAN state.

## 4) FUOTA Plan (ChirpStack v4)

Campaign policy:
- Cadence: `emergency-only`
- Maximum update-window downtime: `30 minutes`

## Phase A - Readiness

- Confirm partition layout and OTA image limits.
- Add firmware version metadata and startup self-test hooks.

## Phase B - Infrastructure

- Deploy and validate ChirpStack FUOTA integration.
- Configure multicast and fragmentation settings for test group.

## Phase C - Device Support

- Implement FUOTA package handlers supported by chosen LoRaWAN stack.
- Add fragment timeout/retry and integrity checks.

## Phase D - Rollout

- Run lab pilot (1 device), then canary group, then wider rollout.
- Stop rollout automatically on failure-rate threshold breach.
- Enforce 30-minute timeout window per emergency campaign.

## 5) Rollback Requirement

Use `hard requirement` rollback policy:
- Keep previous image available until new image is health-verified.
- Require boot-time self-test and watchdog stability window before confirming new image.
- Auto-revert on failed boot/health check or post-update critical fault.

## 6) Security Controls

- Never commit production keys.
- Provision secrets via environment-specific secure method.
- Redact sensitive fields in logs.
