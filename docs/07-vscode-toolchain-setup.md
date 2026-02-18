# VSCode + ESP-IDF Toolchain Setup (Selected Path)

Selected hardware path:
- `RAK3312` core (`ESP32-S3`)
- `RAK19007` base
- `RAK14000` E-Ink
- `RAK12047` SGP40
- External `BMP280`

## 1) Install Tooling

1. Install VSCode extension: `Espressif IDF`.
2. Use the extension to install ESP-IDF tools.
3. Confirm toolchain works with:
   - `idf.py --version`

## 2) Create Project

1. Create a new ESP-IDF project in `firmware/`.
2. Set target:
   - `idf.py set-target esp32s3`
3. Add project components:
   - display driver (RAK14000 path)
   - I2C sensor drivers (SGP40 + BMP280)
   - LoRaWAN stack integration for SX1262

## 3) Build / Flash / Monitor

- `idf.py build`
- `idf.py -p <serial_port> flash monitor`

## 4) Recommended VSCode Tasks

- `build`
- `flash`
- `monitor`
- `flash_monitor`
- `clean_build`

## 5) Bring-up Order (Must Follow)

1. LED heartbeat
2. E-Ink-only communication
3. I2C scan
4. Sensor reads and filtering
5. OTAA join and uplink
6. Integrated loop tuning
7. FUOTA pilot with rollback validation
