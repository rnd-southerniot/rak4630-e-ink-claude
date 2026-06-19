#!/usr/bin/env bash
# Auto-provision a WisBlock node through the SIoT CRM onboarding workflow, which
# registers the device in ChirpStack (AS923) and yields a DevEUI/AppKey that we
# write to firmware/.env.<board> (consumed by pio/scripts/inject_credentials.py).
#
# The CRM backend is expected to already be running and wired to the target
# ChirpStack (verify with GET /chirpstack/status).
#
# Usage:
#   BOARD=rak3312 CRM_PASSWORD='...' tools/provision-node.sh
# Env overrides:
#   BOARD      (rak4631 [default] or rak3312 — sets product/serial naming + .env.<board>)
#   CRM_BASE   (default http://10.10.8.140:4000)
#   CRM_EMAIL  (default admin@southerneleven.com)
#   CRM_PASSWORD (required; supply via env)
#   DEVEUI / APPKEY  (default: randomly generated)
set -euo pipefail

CRM_BASE="${CRM_BASE:-http://10.10.8.140:4000}"
CRM_EMAIL="${CRM_EMAIL:-admin@southerneleven.com}"
# No hardcoded credential — supply the CRM password via the environment:
#   CRM_PASSWORD='...' tools/provision-node.sh
CRM_PASSWORD="${CRM_PASSWORD:?set CRM_PASSWORD in the environment (do not hardcode)}"

# Board selects the product/serial naming and the per-board credentials file.
#   BOARD=rak4631 (default) → RAK4630 (nRF52840);  BOARD=rak3312 → RAK3312 (ESP32-S3)
BOARD="${BOARD:-rak4631}"
case "$BOARD" in
  rak3312) MODEL="RAK3312"; PRODUCT_CODE="LORA-ENV-RAK3312"
           PRODUCT_NAME="RAK3312 Environmental Node (ESP32-S3 + SX1262)" ;;
  rak4631) MODEL="RAK4630"; PRODUCT_CODE="LORA-ENV-RAK4630"
           PRODUCT_NAME="RAK4630 Environmental Node (nRF52840 + SX1262)" ;;
  *) echo "error: unknown BOARD '$BOARD' (use rak4631 or rak3312)" >&2; exit 1 ;;
esac
LORAWAN_REGION="AS923"
HARDWARE_NAME="ESP32-WROOM-32"   # reused valid FK; identity carried by serial/DevEUI

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ENV_OUT="$REPO_ROOT/firmware/.env.$BOARD"   # per-board; inject_credentials prefers this

# --- helpers ---------------------------------------------------------------
jqget() { python3 -c "import sys,json; d=json.load(sys.stdin); print(${1})" 2>/dev/null; }
api() { # api METHOD PATH [json-body]
  local method="$1" path="$2" body="${3:-}"
  if [[ -n "$body" ]]; then
    curl -s -m 15 -X "$method" "$CRM_BASE$path" \
      -H "Authorization: Bearer $TOKEN" -H "Content-Type: application/json" -d "$body"
  else
    curl -s -m 15 -X "$method" "$CRM_BASE$path" -H "Authorization: Bearer $TOKEN"
  fi
}
die() { echo "ERROR: $*" >&2; exit 1; }

# --- credentials (MSB-first uppercase hex) ---------------------------------
DEVEUI="${DEVEUI:-$(printf '%s' "$(openssl rand -hex 8)" | tr 'a-f' 'A-F')}"
APPKEY="${APPKEY:-$(printf '%s' "$(openssl rand -hex 16)" | tr 'a-f' 'A-F')}"
JOINEUI="0000000000000000"
SERIAL="${MODEL}-ENV-${DEVEUI: -6}"
echo ">> DevEUI=$DEVEUI  AppKey=$APPKEY  serial=$SERIAL"

# --- 1. login --------------------------------------------------------------
TOKEN="$(curl -s -m 15 -X POST "$CRM_BASE/auth/login" -H "Content-Type: application/json" \
  -d "{\"email\":\"$CRM_EMAIL\",\"password\":\"$CRM_PASSWORD\"}" \
  | jqget "d.get('access_token','')")"
[[ -n "$TOKEN" ]] || die "login failed for $CRM_EMAIL"
echo ">> logged in as $CRM_EMAIL"

# --- 2. confirm ChirpStack wiring -----------------------------------------
api GET /chirpstack/status | python3 -m json.tool

# --- 3. ensure LoRaWAN product --------------------------------------------
PRODUCT_ID="$(api GET /products | python3 -c "
import sys,json
d=json.load(sys.stdin); items=d if isinstance(d,list) else d.get('data',[])
print(next((p['id'] for p in items if p.get('code')=='$PRODUCT_CODE'),''))")"
if [[ -z "$PRODUCT_ID" ]]; then
  PRODUCT_ID="$(api POST /products "{\"name\":\"$PRODUCT_NAME\",\"code\":\"$PRODUCT_CODE\",\"description\":\"RAK4630 + RAK14000 E-Ink + BME280, AS923 OTAA\",\"isLorawanProduct\":true,\"lorawanRegion\":\"$LORAWAN_REGION\"}" | jqget "d.get('id','')")"
  [[ -n "$PRODUCT_ID" ]] || die "product create failed"
  echo ">> created product $PRODUCT_CODE ($PRODUCT_ID)"
else
  echo ">> product exists $PRODUCT_CODE ($PRODUCT_ID)"
fi

# --- 3b. ensure product has an SOP template (required before task creation) --
HAS_SOP="$(api GET "/products/$PRODUCT_ID/sop-template" | jqget "d.get('id','')" || true)"
if [[ -z "$HAS_SOP" ]]; then
  api POST "/products/$PRODUCT_ID/sop-template" "{\"productId\":\"$PRODUCT_ID\",\"steps\":[{\"id\":\"step-1\",\"title\":\"Provision & Join\",\"description\":\"OTAA provisioning and join verification\",\"order\":1}],\"version\":1}" >/dev/null
  echo ">> created SOP template for $PRODUCT_CODE"
fi

# --- 4. resolve hardware id -----------------------------------------------
HARDWARE_ID="$(api GET /hardware-catalog | python3 -c "
import sys,json
d=json.load(sys.stdin); items=d if isinstance(d,list) else d.get('data',[])
print(next((h['id'] for h in items if h.get('name')=='$HARDWARE_NAME'),''))")"
[[ -n "$HARDWARE_ID" ]] || die "hardware '$HARDWARE_NAME' not found in catalog"
echo ">> hardwareId=$HARDWARE_ID ($HARDWARE_NAME)"

# --- 5. create onboarding task --------------------------------------------
TASK_ID="$(api POST /workflow/tasks "{\"clientName\":\"SIoT R&D Lab\",\"clientEmail\":\"rnd@southerniot.com\",\"clientPhone\":\"+8801700000000\",\"clientAddress\":\"Dhaka, Bangladesh\",\"contactPerson\":\"Arif\",\"productId\":\"$PRODUCT_ID\"}" | jqget "d.get('id','')")"
[[ -n "$TASK_ID" ]] || die "task create failed"
echo ">> task created: $TASK_ID"

# --- 6. walk workflow statuses --------------------------------------------
adv() { # adv STATUS BODY
  echo ">> -> $1"
  local resp; resp="$(api PUT "/workflow/tasks/$TASK_ID/status/$1" "$2")"
  local cur; cur="$(printf '%s' "$resp" | jqget "d.get('currentStatus', d.get('status',''))")"
  [[ "$cur" == "$1" || -n "$cur" ]] || { echo "$resp"; die "transition to $1 failed"; }
}
adv SCHEDULED_VISIT '{"scheduledDate":"2026-06-20T04:00:00Z"}'
adv REQUIREMENTS_COMPLETE '{"reportData":{"signalStrength":-75,"installationLocation":"lab-bench","notes":"bring-up"}}'
adv HARDWARE_PROCUREMENT_COMPLETE "{\"hardwareList\":[{\"hardwareId\":\"$HARDWARE_ID\",\"quantity\":1,\"notes\":\"RAK4630 node\"}]}"
adv HARDWARE_PREPARED_COMPLETE "{\"deviceList\":[{\"hardwareId\":\"$HARDWARE_ID\",\"deviceSerial\":\"$SERIAL\",\"firmwareVersion\":\"gate-bringup\",\"devEui\":\"$DEVEUI\",\"appKey\":\"$APPKEY\",\"notes\":\"AS923 OTAA bring-up\"}]}"

# pre-installation checklist must be completed via its own endpoint before READY_FOR_INSTALLATION
echo ">> completing pre-installation checklist"
api PUT "/workflow/tasks/$TASK_ID/pre-installation-checklist" '{"devicesTestComplete":true,"firmwareLoaded":true,"qrCodesPrinted":true,"clientConfirmedDate":true,"accessArranged":true,"contactAvailable":true,"installationGuide":true,"networkConfig":true,"credentialsReady":true}' >/dev/null

adv READY_FOR_INSTALLATION '{}'

# --- 7. verify ChirpStack registration ------------------------------------
echo ">> verifying device in ChirpStack..."
sleep 2
api GET "/chirpstack/device/$(printf '%s' "$DEVEUI" | tr 'A-F' 'a-f')" | python3 -m json.tool

# --- 8. emit per-board firmware/.env.<board> -------------------------------
umask 077
cat > "$ENV_OUT" <<EOF
# Auto-provisioned via tools/provision-node.sh (CRM onboarding workflow -> ChirpStack AS923)
# Board: $BOARD ($MODEL)  Task: $TASK_ID  Serial: $SERIAL
DEVEUI=$DEVEUI
APPKEY=$APPKEY
JOINEUI=$JOINEUI
EOF
echo ">> wrote $ENV_OUT"
echo ">> DONE. DevEUI=$DEVEUI"
