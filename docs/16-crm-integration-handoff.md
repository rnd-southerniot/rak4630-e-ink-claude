# CRM Integration Handoff — Firmware Auto-Provisioning

**From:** Firmware / R&D (WisBlock environmental node — RAK4630 + RAK3312)
**To:** CRM platform team (`siot-crm-review`)
**Date:** 2026-06-20
**Purpose:** Document how our firmware bring-up tooling auto-provisions LoRaWAN nodes
**through your CRM onboarding workflow**, so you know which API surface we depend on and
why. Nothing here asks you to change anything — it's a contract description so the
integration stays stable as the CRM evolves.

---

## 1. What we built

We provision a physical node **end-to-end through the CRM**, not by hitting ChirpStack
directly. A single script — `tools/provision-node.sh` in the firmware repo — logs into the
CRM, walks an onboarding **task** through its workflow states, and relies on the CRM to
register the device in ChirpStack (AS923) when the task reaches the right state. The script
then reads back the generated **DevEUI / AppKey** and writes them to a gitignored
`firmware/.env.<board>` that the firmware build injects as OTAA credentials.

```
provision-node.sh ──REST──> CRM (10.10.8.140:4000) ──> ChirpStack (AS923)
        ▲                                                      │
        └──────────── DevEUI / AppKey read back ───────────────┘
                      → firmware/.env.<board> → flashed to node
```

The key design point: **the CRM is the system of record for device identity.** We treat the
onboarding workflow as the provisioning API. We deliberately do **not** create ChirpStack
devices ourselves.

---

## 2. API surface we call (the contract)

All calls are Bearer-authenticated REST against `CRM_BASE` (default
`http://10.10.8.140:4000`). In order:

| # | Method & path | Why we call it / fields we depend on |
|---|---------------|--------------------------------------|
| 1 | `POST /auth/login` | `{email, password}` → we read `access_token`. |
| 2 | `GET /chirpstack/status` | Pre-flight: confirm CRM↔ChirpStack wiring before provisioning. |
| 3 | `GET /products` | Find our product by `code` (`LORA-ENV-RAK4630` / `LORA-ENV-RAK3312`). Response may be a bare array or `{data:[…]}` — we handle both. |
| 4 | `POST /products` | Create if absent. We send `isLorawanProduct:true` + `lorawanRegion:"AS923"` — **this is what marks the product as LoRaWAN and drives ChirpStack provisioning downstream.** |
| 5 | `GET /products/{id}/sop-template` | Check the product has an SOP template. |
| 6 | `POST /products/{id}/sop-template` | Create one if missing (see §3, gotcha 1). |
| 7 | `GET /hardware-catalog` | Resolve a `hardwareId` by `name`. |
| 8 | `POST /workflow/tasks` | Create the onboarding task (`clientName/Email/Phone/Address`, `contactPerson`, `productId`) → we read `id`. |
| 9 | `PUT /workflow/tasks/{id}/status/{STATUS}` | Advance through the workflow (see §4). We read `currentStatus` (or `status`) from the response to confirm each transition. |
| 10 | `PUT /workflow/tasks/{id}/pre-installation-checklist` | Complete the checklist (see §3, gotcha 2). |
| 11 | `GET /chirpstack/device/{devEui}` | Verify the device exists in ChirpStack. **DevEUI must be lowercased** in this path. |

**The credential-bearing call is #9 at `HARDWARE_PREPARED_COMPLETE`.** Its body carries the
`deviceList`, and this is where DevEUI/AppKey enter the system:

```json
{
  "deviceList": [{
    "hardwareId":    "<from hardware-catalog>",
    "deviceSerial":  "RAK3312-ENV-4DBB79",
    "firmwareVersion": "gate-bringup",
    "devEui":        "CBAE62F5314DBB79",
    "appKey":        "E19E90D43DD911F18A4923A8DAE4AA81",
    "notes":         "AS923 OTAA bring-up"
  }]
}
```

We generate DevEUI (8-byte) and AppKey (16-byte) as **uppercase hex** client-side, and
expect ChirpStack to be provisioned with exactly these values (JoinEUI defaults all-zero).

---

## 3. Prerequisites we discovered (worth documenting on your side)

These weren't obvious from the outside and cost us bring-up time. Flagging them so they're
intentional, not accidental, going forward:

1. **A product must have an SOP template before a task can be created against it.** Task
   creation fails otherwise. Our script defensively `POST`s a minimal one-step template if
   `GET /products/{id}/sop-template` returns none.
2. **`READY_FOR_INSTALLATION` requires the pre-installation checklist to be completed via its
   own endpoint first** (`PUT …/pre-installation-checklist` with all flags true). The status
   transition is rejected if the checklist isn't satisfied. This is a separate call from the
   status PUT, which is easy to miss.

If these gates are intended, great — we've adapted to them. If they're incidental, a documented
"provisioning happy-path" contract would let us depend on them confidently.

---

## 4. Workflow state sequence we drive

```
(task created)
   → SCHEDULED_VISIT                 { scheduledDate }
   → REQUIREMENTS_COMPLETE           { reportData: { signalStrength, installationLocation, notes } }
   → HARDWARE_PROCUREMENT_COMPLETE   { hardwareList: [{ hardwareId, quantity, notes }] }
   → HARDWARE_PREPARED_COMPLETE      { deviceList: [{ …devEui, appKey… }] }   ← credentials in
   → (pre-installation-checklist)    { all flags true }
   → READY_FOR_INSTALLATION          {}                                       ← triggers ChirpStack registration
```

Our working assumption: **device registration in ChirpStack happens at / by the time the task
reaches `READY_FOR_INSTALLATION`.** We `GET /chirpstack/device/{devEui}` right after to confirm.
If that trigger point ever moves, our verification step (and the firmware flash that follows)
breaks — so please treat it as part of the contract.

---

## 5. Multi-board / multi-device notes

- We now provision **one CRM product + one device per board model**:
  `LORA-ENV-RAK4630` and `LORA-ENV-RAK3312`. The script's `BOARD` parameter selects product
  code/name and device serial (`<MODEL>-ENV-<last 6 of DevEUI>`).
- **Each physical node is its own device** (its own DevEUI/AppKey). One product can hold many
  devices; we create a fresh task per node.
- Example devices currently provisioned: RAK4630 `F815C58DC40AC898`, RAK3312
  `CBAE62F5314DBB79` (both AS923).

---

## 6. What would help us most from the CRM side

In rough priority order — all optional, none blocking:

1. **A stable, documented provisioning contract** for the 11 calls above (especially the two
   prerequisites in §3 and the registration trigger point in §4).
2. **An idempotent or "find-or-create device" path.** Re-running provisioning currently risks
   accumulating tasks/devices; a way to look up an existing device by serial/DevEUI and reuse
   it would keep records clean.
3. **Explicit confirmation in the API response that ChirpStack registration succeeded** (e.g.
   the device id / status echoed back on `READY_FOR_INSTALLATION`), so we don't have to poll
   `GET /chirpstack/device/{devEui}` separately.
4. Clarity on whether `hardware-catalog` entries are meant to model exact modules — we
   currently reuse a valid catalog entry and carry true identity in the serial/DevEUI.

---

## 7. Reference

- Firmware-side script: `tools/provision-node.sh` (the authoritative list of calls/payloads).
- Our internal workflow docs: `docs/12-auto-provisioning-workflow.md`,
  `docs/13-crm-sop-template-and-product-setup.md`.
- CRM: `http://10.10.8.140:4000` · ChirpStack: `10.10.8.140` · Region: AS923-1 (Bangladesh).

Happy to walk through any of this or adjust the script to match a contract you'd prefer.
