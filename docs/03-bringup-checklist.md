# Bring-up and Verification Checklist

Use this as the implementation gate for every firmware revision.

## Phase 1 - Baseline Build/Flash/Monitor

1. Compile firmware in clean environment.
2. Flash target board.
3. Start serial monitor and verify boot banner.

Pass criteria:
- Build succeeds with zero linker errors.
- Boot log prints firmware version, board ID, build timestamp.

Evidence:
- Serial log snippet attached to commit/PR notes.

## Phase 2 - LED Heartbeat

1. Run non-blocking heartbeat task (1 Hz recommended).
2. Keep system running for at least 10 minutes.
3. Optional reusable command:
   - `/Users/arif/rak4630-e-ink/examples/gates/run_gate_1_heartbeat.sh /dev/cu.usbmodem1101`

Pass criteria:
- Stable blink, no resets, no watchdog trip.

## Phase 3 - E-Ink Only Communication Test

1. Disable all sensor and LoRa tasks.
2. Initialize RAK14000 and draw test pattern.
3. Refresh display with monotonic counter.

Pass criteria:
- Frame updates are deterministic.
- No bus errors or panel init failures.

## Phase 3.5 - I2C Smoke (Gate 2.1 Equivalent)

1. Run I2C-only smoke check after E-Ink pass.
2. Scan I2C bus and probe selected device profile (`SGP40`, `BMP280`, or both).

Pass criteria:
- I2C scan completes with deterministic addresses.
- Selected expected I2C devices are detected.

## Phase 4 - I2C Scan (Full Presence)

1. Enable I2C master and scan full address range.
2. Verify `SGP40` and `BMP280` are discovered.

Pass criteria:
- Expected devices always detected.
- No random transient addresses.

## Phase 5 - Sensor Read and Processing

1. Read VOC (`SGP40`) and pressure/temperature (`BMP280`) periodically.
2. Apply plausibility/range checks.
3. Apply filtering and convert to engineering units.

Pass criteria:
- Sensor read success >= 99% in test run.
- Invalid samples are rejected and logged.

## Phase 6 - LoRaWAN OTAA (AS923-1) + ChirpStack v4

1. Provision `DevEUI/JoinEUI/AppKey`.
2. Configure region profile to `AS923-1`.
3. Perform OTAA join and send test payload.

Pass criteria:
- Join accepted by ChirpStack v4.
- Uplink decoded values match local values.

## Phase 7 - Combined Flow

1. Execute full loop: sensor read -> process -> display -> uplink.
2. Run for a minimum 24-hour soak.

Pass criteria:
- No fatal reset.
- Controlled retry behavior on join/uplink failure.
- Error counters remain within expected bounds.

## Phase 8 - Refresh Time Optimization

1. Measure full/partial refresh durations and current impact.
2. Adjust update interval and threshold-based redraw logic.

Pass criteria:
- Display remains readable with minimal ghosting.
- Power/latency tradeoff documented and accepted.

## Phase 9 - FUOTA + Rollback

1. Execute FUOTA campaign on lab node.
2. Validate health-check-based image confirmation.
3. Force one negative test to verify automatic rollback.

Pass criteria:
- Update completes successfully in normal case.
- Rollback succeeds in forced-failure case.
