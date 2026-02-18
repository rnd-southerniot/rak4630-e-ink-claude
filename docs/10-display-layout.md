# Display Layout Specification (Landscape)

Panel mode: landscape
Refresh mode: threshold-triggered only

## Field Priority (Highest to Lowest)

1. `LoRaWAN status` (`JOINED`, `JOINING`, `OFFLINE`)
2. `VOC index` (SGP40-derived)
3. `Pressure` (hPa)
4. `Temperature` (C)
5. `Battery` (V)
6. `Last uplink time` (relative seconds/minutes)

## Threshold-to-Refresh Rules

A display refresh is triggered when any one condition is met:
- VOC change >= 3.00
- Pressure change >= 50 Pa
- Temperature change >= 0.50 C

Additionally:
- Enforce minimum gap of 30 seconds between refreshes.

## Fail-safe Rendering

If sensors are unavailable:
- Keep last valid values
- Show status banner: `SENSOR ERROR`
- Show LoRaWAN status independently
