# Information Needed (Resolved and Open)

## Resolved Inputs

- Platform path: `ESP-IDF`
- Core module strategy: ESP32-based WisBlock core (`RAK3312`)
- VOC sensor: `SGP40` (`RAK12047`)
- Pressure sensor: external `BMP280` on I2C (`3.3V` native)
- Region: `AS923-1`
- Network server major version: `ChirpStack v4`
- Gateway: `RAK7266` using `Semtech UDP packet forwarder`
- OTAA credential policy: `per-device`
- OTAA `JOINEUI`: default `0000000000000000`
- Runtime interval: `5 minutes` (sampling + uplink)
- Deployment country: `Bangladesh`
- ChirpStack deployment: `Docker`
- Display refresh policy: redraw only on value-change threshold
- Display orientation: `landscape`
- Display field priority: status, VOC, pressure, temperature, battery, last uplink
- Gate split policy: `Gate 2` (E-Ink SPI smoke) and `Gate 2.1` (I2C smoke, internal id `21`)
- FUOTA cadence: `emergency-only`
- FUOTA max downtime: `30 minutes`
- FUOTA rollback: `hard requirement`

## Recommended Slot Mapping

- Core socket: `RAK3312`
- IO slot: `RAK14000`
- Slot A: `RAK12047` (SGP40)
- I2C header: external `BMP280`

## Open Inputs Required Before Coding

- Per-device `DEVEUI` values for deployed nodes.
- Per-device `APPKEY` values for deployed nodes.

## Delivery / Process

1. Preferred repository structure for firmware implementation.
2. Required CI gates (format/lint/unit/integration).
3. Required evidence format for test reports.
