# Data Flow Diagram

## Runtime Data Path

```mermaid
flowchart TD
    A["Boot / Reset"] --> B["Board Init\nclocks, GPIO, buses"]
    B --> C["Heartbeat Task"]
    B --> D["E-Ink Init"]
    B --> E["I2C Scan"]
    E --> F{"Expected sensors found?"}
    F -- No --> F1["Raise fault flag\nshow error on display\nretry scan"] --> E
    F -- Yes --> G["Sensor Sampling Task\nVOC + Pressure/Temp"]
    G --> H["Data Quality Checks\nrange + stale + CRC(if any)"]
    H --> I{"Valid sample?"}
    I -- No --> I1["Increment error counter\nskip uplink"] --> G
    I -- Yes --> J["Filter / Process\nEWMA or moving average"]
    J --> K["Render Frame Model"]
    K --> L["E-Ink Refresh\nfull or partial"]
    J --> M["Payload Encode\ncompact binary"]
    M --> N["LoRaWAN Stack\nOTAA session"]
    N --> O{"Joined?"}
    O -- No --> O1["Rejoin with backoff"] --> N
    O -- Yes --> P["Uplink to Gateway"]
    P --> Q["ChirpStack v4 NS/AS"]
    Q --> R["Decoder / App Integration"]
    R --> S["Telemetry Storage / Dashboard"]
```

## Gate Control Path (Canonical)

```mermaid
flowchart TD
    G0["Gate 0 env"] --> G1["Gate 1 heartbeat"]
    G1 --> G2["Gate 2 display_smoke (SPI E-Ink)"]
    G2 --> G21["Gate 2.1 i2c_smoke (I2C only, id=21)"]
    G21 --> G3["Gate 3 i2c_presence"]
    G3 --> G4["Gate 4 sensor_pipeline"]
    G4 --> G5["Gate 5 payload_v1"]
    G5 --> G6["Gate 6 lorawan_join_uplink"]
    G6 --> G7["Gate 7 reliability_buffer"]
    G7 --> G8["Gate 8 fuota_scaffold"]
    G8 --> G9["Gate 9 live_publish"]
```

Control policy:
- Run one gate per flash cycle.
- On PASS, firmware logs `result=PASS gate=<id>` and halts progression.
- Operator must change `CONFIG_APP_GATE` and reflash before next gate.

## Control and Timing Notes

- Display refresh is decoupled from sensor sampling; update on data-change threshold and max interval.
- Uplink scheduler should enforce AS923-1 duty-cycle and fair access limits.
- Join/uplink retries must use capped exponential backoff.
- Error counters and last-fault reason should be visible in serial logs and a debug screen.

## FUOTA Control Path (Future)

```mermaid
flowchart LR
    A["Firmware Build + Signed Artifact"] --> B["FUOTA Server"]
    B --> C["ChirpStack Multicast / Fragmentation"]
    C --> D["Node Receives Fragments"]
    D --> E["Integrity Check"]
    E --> F{"Valid image?"}
    F -- Yes --> G["Activate New Firmware"]
    F -- No --> H["Rollback / Keep Current"]
```
