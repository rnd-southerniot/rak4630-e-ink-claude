# Resource Review (Official References)

Reviewed on: 2026-02-13

## Core and Base Board

- RAK3312 Overview: https://docs.rakwireless.com/product-categories/wisblock/rak3312/overview/
  - Key takeaway: ESP32-S3 + SX1262 integrated in a WisBlock Core module.
- RAK19007 Overview: https://docs.rakwireless.com/product-categories/wisblock/rak19007/overview/
  - Key takeaway: Base-board slot and external header context.

## Display

- RAK14000 Overview: https://docs.rakwireless.com/product-categories/wisblock/rak14000/overview/
  - Key takeaway: E-Ink module usage and IO-slot integration constraints.

## Sensors

- RAK12047 Overview: https://docs.rakwireless.com/product-categories/wisblock/rak12047/overview/
  - Key takeaway: SGP40 VOC sensor module for WisBlock.
- BMP280 Datasheet (Bosch): https://www.bosch-sensortec.com/products/environmental-sensors/pressure-sensors/bmp280/
  - Key takeaway: pressure/temperature sensor characteristics for external I2C integration.

## LoRaWAN Stack and FUOTA

- ChirpStack documentation: https://www.chirpstack.io/docs/chirpstack/
- ChirpStack FUOTA guide: https://www.chirpstack.io/docs/chirpstack/use/fuota.html
- ChirpStack FUOTA server repository: https://github.com/chirpstack/chirpstack-fuota-server

## Design Inference Used

From official module docs:
- `RAK14000` uses the IO-slot path.
- Choosing a core with integrated LoRa (`RAK3312`) avoids needing a separate LPWAN IO module, preventing IO-slot contention.
