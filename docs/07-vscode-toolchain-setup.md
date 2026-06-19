# VSCode + PlatformIO Toolchain Setup (Selected Path)

Selected hardware path:
- `RAK4630` core (`nRF52840` + SX1262)
- `RAK19007` base
- `RAK14000` E-Ink
- `RAK12047` SGP40
- External `BMP280`

## 1) Install Tooling

1. Install VSCode extension: `PlatformIO IDE`.
2. Let the extension install the PlatformIO core and platform toolchains on first launch.
3. Confirm the toolchain works with:
   - `~/.platformio/penv/bin/pio --version`

The first build performs a one-time board/variant install (the `WisCore_RAK4631_Board` variant for the RAK4630), as described in the `pio/platformio.ini` header. No `set-target` step is required; the board and framework are pinned in `pio/platformio.ini`.

## 2) Project Layout

1. The PlatformIO project lives in `pio/`.
2. Sources are in `pio/src/`, headers in `pio/include/`, board variant in `pio/variants/WisCore_RAK4631_Board/`.
3. Subsystems already wired in:
   - display driver (RAK14000 / SSD1680 path)
   - I2C sensor drivers (SGP40 + BMP280)
   - LoRaWAN stack integration for SX1262 (SX126x-Arduino, OTAA, AS923-1)

## 3) Build / Flash / Monitor

Run from the `pio/` directory (monitor baud `115200`):

- `~/.platformio/penv/bin/pio run` (build)
- `~/.platformio/penv/bin/pio run -t upload -t monitor` (flash + monitor)
- `~/.platformio/penv/bin/pio run -t clean && ~/.platformio/penv/bin/pio run` (clean build)

Gate selection is done by editing `-DAPP_GATE=N` in `pio/platformio.ini`, then rebuilding.

## 4) Recommended VSCode / PlatformIO Tasks

- `Build` (`pio run`)
- `Upload` (`pio run -t upload`)
- `Monitor` (`pio device monitor`)
- `Upload and Monitor` (`pio run -t upload -t monitor`)
- `Clean` (`pio run -t clean`)

## 5) Bring-up Order (Must Follow)

1. LED heartbeat
2. E-Ink-only communication
3. I2C scan
4. Sensor reads and filtering
5. OTAA join and uplink
6. Integrated loop tuning
7. FUOTA pilot with rollback validation
