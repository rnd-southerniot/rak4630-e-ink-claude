---
name: rak4630-lorawan-provisioning
description: Auto-provision a RAK4630 LoRaWAN node via the SIoT CRM onboarding workflow, write firmware/.env, and validate with gates 6/7/9. Use when onboarding/registering a node onto ChirpStack (AS923).
---

# RAK4630 LoRaWAN Provisioning

Provision a node's OTAA credentials through the CRM (which registers it in ChirpStack), then
prove it on hardware. Full background: `docs/12-auto-provisioning-workflow.md`.

## 1. Provision (CRM workflow → firmware/.env)

```bash
CRM_PASSWORD='<crm-password>' tools/provision-node.sh
```

This logs into the CRM at `10.10.8.140:4000`, ensures the LoRaWAN product + SOP template,
creates+advances an onboarding task, registers the device in ChirpStack at
`READY_FOR_INSTALLATION`, and writes `firmware/.env` (`DEVEUI`/`APPKEY`/`JOINEUI`).

Prereqs (see `docs/13-...`): a LoRaWAN product **with an SOP template** and a hardware-catalog
entry. The script creates the product+SOP idempotently.

## 2. Verify registration

```bash
curl -s "$BASE/chirpstack/device/<deveui-lowercase>" -H "Authorization: Bearer $TOKEN"   # found:true
```

Confirm `lorawanProvisioningStatus == COMPLETED` on the task and the device shows in the
ChirpStack UI.

## 3. Build + flash gate 6 (real join + confirmed uplink)

```bash
examples/gates/set_gate_new.sh 6
cd pio && ~/.platformio/penv/bin/pio run            # expect: inject_credentials: injected APP_DEVEUI...
~/.platformio/penv/bin/pio run -t upload --upload-port /dev/cu.usbmodem1101
```

Capture serial (see `rak4630-serial-capture` skill) and confirm:
- `LORAWAN: join_success` (gateway is live)
- `LORAWAN: uplink_sent_confirmed` → `uplink_ack_ok` (confirmed ACK)
- `result=PASS gate=6`
- uplink frame visible in the ChirpStack device event log

Then gate 7 (buffer/flush) and gate 9 (`--gate9-devices 2`, sensor→display→confirmed uplink).

## Gotchas
- Two ChirpStacks: target is **10.10.8.140** (Docker), not 10.10.8.171 (K8s) in the repo .env.
- `firmware/.env` is gitignored — never commit it.
- AS923 duty cycle: confirmed uplinks must be spaced; gate-9 uplink period is already raised.
