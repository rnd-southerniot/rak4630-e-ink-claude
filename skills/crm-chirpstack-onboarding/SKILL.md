---
name: crm-chirpstack-onboarding
description: Create the SIoT CRM prerequisites (LoRaWAN product, SOP template, hardware entry) and drive the onboarding workflow that registers a device in ChirpStack. Use when setting up CRM products or debugging task-creation/provisioning failures.
---

# CRM ChirpStack Onboarding

Operational checklist for the CRM side of provisioning. Reference:
`docs/13-crm-sop-template-and-product-setup.md` and `docs/12-auto-provisioning-workflow.md`.

## Auth + status

```bash
BASE=http://10.10.8.140:4000
TOKEN=$(curl -s -X POST $BASE/auth/login -H 'Content-Type: application/json' \
  -d '{"email":"admin@southerneleven.com","password":"<pw>"}' \
  | python3 -c 'import sys,json;print(json.load(sys.stdin)["access_token"])')
curl -s $BASE/chirpstack/status -H "Authorization: Bearer $TOKEN"   # which ChirpStack + IDs
```

`/chirpstack/status` confirms the backend points at `grpc://chirpstack:8080` (the local 140
ChirpStack) and has tenant/application/device-profile IDs.

## Create prerequisites

1. **LoRaWAN product** — `POST /products` with `isLorawanProduct:true`, `lorawanRegion:"AS923"`.
2. **SOP template** — `POST /products/{id}/sop-template` with at least one `SOPStepDto`.
   **Required:** without it, `POST /workflow/tasks` returns 404 "SOP template not found".
3. **Hardware entry** — reuse one from `GET /hardware-catalog` or create a RAK4630 entry.

## Drive the workflow

`POST /workflow/tasks` → walk statuses with `PUT /workflow/tasks/{id}/status/{STATUS}`:
- `HARDWARE_PREPARED_COMPLETE`: send `deviceList:[{hardwareId,deviceSerial,firmwareVersion,devEui,appKey}]`.
- Set the checklist via its **own** endpoint: `PUT /workflow/tasks/{id}/pre-installation-checklist` (9 items true).
- `READY_FOR_INSTALLATION`: triggers ChirpStack registration.

## Troubleshooting
- **404 SOP template** → product has no SOP template (step 2).
- **"checklist incomplete (0/9)"** → you passed it in the status body; use the checklist endpoint.
- **Device not in ChirpStack** → check `lorawanProvisioningStatus`/`lorawanProvisioningError`
  on the task; verify the product is `isLorawanProduct` and devices have devEui+appKey.
- Wrong server: the repo `.env` references 10.10.8.171 (K8s); the live backend uses 140.

`tools/provision-node.sh` automates all of the above.
