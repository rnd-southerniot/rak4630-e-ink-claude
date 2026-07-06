# Auto-Provisioning Workflow (CRM → ChirpStack → Firmware)

> **⚠ Historical (pre-P6 / PlatformIO-nRF52).** This doc predates the ESP-IDF/ESP32-S3 consolidation and may reference the retired `pio/` tree, `-DAPP_GATE`, or SX126x-Arduino. Canonical build/flash/provision instructions now live in [CLAUDE.md](../CLAUDE.md) (ESP-IDF `firmware/` + Kconfig).

How a RAK4630 LoRaWAN node gets its OTAA credentials **automatically** through the SIoT CRM
onboarding workflow, and how those credentials reach the firmware. This is the proven
proof-of-concept (validated end-to-end: real OTAA join + confirmed uplink on gates 6/7/9).

## Overview

```
 generate DevEUI/AppKey (host)
            │
            ▼
 ┌──────────────────────────┐   CRM onboarding workflow      ┌──────────────────┐
 │  tools/provision-node.sh │ ─────────────────────────────► │  CRM backend     │
 │  (REST calls to CRM)     │   product → SOP → task →        │  10.10.8.140:4000│
 └──────────────────────────┘   HARDWARE_PREPARED →           └────────┬─────────┘
            │                    READY_FOR_INSTALLATION                  │ gRPC
            │ writes                                                     ▼
            ▼                                                    ┌──────────────────┐
   firmware/.env  ── inject_credentials.py (pre-build) ──►       │  ChirpStack v4   │
   DEVEUI/APPKEY/JOINEUI      -DAPP_DEVEUI / -DAPP_APPKEY         │  AS923, app+prof │
            │                                                    └────────┬─────────┘
            ▼                                                             │ OTAA join
   firmware build (pio)  ──flash──►  RAK4630  ──join+uplink──►  RAK7266 gateway ─►
```

## The two ChirpStack servers (don't mix them up)

| Server | Address | Role | Notes |
|--------|---------|------|-------|
| **Docker (target)** | `10.10.8.140` (gRPC `chirpstack:8080`) | The CRM backend at `:4000` talks to this | Has its **own** tenant/app/profile IDs + token |
| K8s (other) | `10.10.8.171:30808` | What the CRM **repo** `.env` references | Different IDs/token — using these against 140 fails |

The live CRM backend (`http://10.10.8.140:4000`) is wired to the local Docker ChirpStack
(`grpc://chirpstack:8080`) — confirm any time with `GET /chirpstack/status`.

## Prerequisites (one-time, see doc 13)

1. CRM backend running at `10.10.8.140:4000` (it already is).
2. A **LoRaWAN product** (`isLorawanProduct=true`, `lorawanRegion=AS923`) **with an SOP
   template** — task creation 404s without the SOP template.
3. A hardware-catalog entry to reference as `hardwareId`.
4. ChirpStack has the AS923 application + device-profile the backend is configured for
   (`GET /chirpstack/status` shows the IDs).

`tools/provision-node.sh` creates the product + SOP template idempotently; doc 13 covers the
manual steps and the "why".

## Run it

```bash
CRM_PASSWORD='<crm-password>' tools/provision-node.sh
```

What it does (all REST against `10.10.8.140:4000`, auth = seeded CRM account):

1. Generates `DEVEUI` (8 bytes) + `APPKEY` (16 bytes), uppercase hex.
2. `POST /auth/login` → bearer token.
3. Ensures the LoRaWAN product + its SOP template.
4. `POST /workflow/tasks` (client + product) → `taskId`.
5. Walks the status lifecycle:
   `SCHEDULED_VISIT → REQUIREMENTS_COMPLETE → HARDWARE_PROCUREMENT_COMPLETE →
   HARDWARE_PREPARED_COMPLETE` — DevEUI/AppKey are supplied here in `deviceList[]`.
6. `PUT /workflow/tasks/{id}/pre-installation-checklist` (all 9 items true) — required, and
   **not** settable via the status-transition body.
7. `PUT …/status/READY_FOR_INSTALLATION` — this is the trigger:
   `WorkflowService.provisionLorawanDevices()` → `ChirpStackService.provisionTask()` →
   `DeviceService.create` + `createKeys` (registers the device + OTAA keys in ChirpStack).
8. Writes `firmware/.env` (`DEVEUI`, `APPKEY`, `JOINEUI=0000000000000000`).

> The status that fires ChirpStack registration is **`READY_FOR_INSTALLATION`**, gated on
> `product.isLorawanProduct` and devices having both `devEui` and `appKey`. For the AS923
> LoRaWAN-1.0.3 profile only the AppKey matters (NwkKey is set equal to AppKey).

## Credentials → firmware

`firmware/.env` (gitignored) is read by `pio/scripts/inject_credentials.py` (a PlatformIO
pre-build hook) which emits `-DAPP_DEVEUI / -DAPP_APPKEY / -DAPP_JOINEUI`. A successful build
prints `inject_credentials: injected APP_DEVEUI, APP_JOINEUI, APP_APPKEY`. Keys never enter
source control. DevEUI/JoinEUI are stored MSB-first to match the ChirpStack console and the
firmware's hex parser.

## Verify

1. **CRM/ChirpStack**: `GET /chirpstack/device/<devEui-lowercase>` → `found:true`; the task's
   `lorawanProvisioningStatus == COMPLETED`; the device appears in the ChirpStack UI under the
   configured application.
2. **Firmware (hardware)** — flash gate 6 and confirm:
   - `inject_credentials: injected …`
   - `LORAWAN: join_success …` (proves the RAK7266 gateway is live)
   - `LORAWAN: uplink_sent_confirmed` → `LORAWAN: uplink_ack_ok` (confirmed downlink ACK)
   - `result=PASS gate=6`
   - The uplink frame + 12-byte payload appear in the ChirpStack device **event log**.
   Then gate 7 (buffer/flush) and gate 9 (sensor→display→confirmed uplink).

See `skills/rak4630-lorawan-provisioning/` for the operational checklist and
`docs/GATE_EXECUTION_LOG.md` for captured evidence.

## Related

- `docs/13-crm-sop-template-and-product-setup.md` — the CRM prerequisites and SOP templates.
- `tools/provision-node.sh` — the orchestration script (the deliverable).
- `tools/serial_capture.py` — non-interactive serial capture for verifying gates.
