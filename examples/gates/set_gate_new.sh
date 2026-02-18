#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <gate:0..9|2.1|21> [--gate2p1-devices 1|2|3] [--gate3-devices 1|2|3] [--gate4-devices 1|2|3] [--gate9-devices 1|2|3]"
  exit 1
fi

GATE_INPUT="$1"
shift || true

GATE="$GATE_INPUT"
if [[ "$GATE_INPUT" == "2.1" ]]; then
  GATE="21"
fi

if ! [[ "$GATE" =~ ^[0-9]+$ ]] || { [[ "$GATE" -gt 9 ]] && [[ "$GATE" -ne 21 ]]; }; then
  echo "invalid gate: $GATE_INPUT (allowed 0..9, 2.1, or 21)"
  exit 1
fi

SDK="/Users/arif/rak4630-e-ink/firmware/sdkconfig"

set_value() {
  local key="$1"
  local value="$2"
  if rg -q "^${key}=" "$SDK"; then
    perl -pi -e "s/^${key}=.*/${key}=${value}/" "$SDK"
  else
    echo "${key}=${value}" >> "$SDK"
  fi
}

set_value CONFIG_APP_GATE "$GATE"

# force canonical scheme
if rg -q '^# CONFIG_APP_GATE_ID_SCHEME_NEW is not set$' "$SDK"; then
  perl -pi -e 's/^# CONFIG_APP_GATE_ID_SCHEME_NEW is not set$/CONFIG_APP_GATE_ID_SCHEME_NEW=y/' "$SDK"
elif ! rg -q '^CONFIG_APP_GATE_ID_SCHEME_NEW=y$' "$SDK"; then
  echo 'CONFIG_APP_GATE_ID_SCHEME_NEW=y' >> "$SDK"
fi

if rg -q '^CONFIG_APP_GATE_ID_SCHEME_LEGACY=y$' "$SDK"; then
  perl -pi -e 's/^CONFIG_APP_GATE_ID_SCHEME_LEGACY=y/# CONFIG_APP_GATE_ID_SCHEME_LEGACY is not set/' "$SDK"
elif ! rg -q '^# CONFIG_APP_GATE_ID_SCHEME_LEGACY is not set$' "$SDK"; then
  echo '# CONFIG_APP_GATE_ID_SCHEME_LEGACY is not set' >> "$SDK"
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    --gate2p1-devices)
      set_value CONFIG_APP_GATE2P1_EXPECTED_DEVICES "$2"; shift 2 ;;
    --gate3-devices)
      set_value CONFIG_APP_GATE3_EXPECTED_DEVICES "$2"; shift 2 ;;
    --gate4-devices)
      set_value CONFIG_APP_GATE4_EXPECTED_DEVICES "$2"; shift 2 ;;
    --gate9-devices)
      set_value CONFIG_APP_GATE9_EXPECTED_DEVICES "$2"; shift 2 ;;
    *)
      echo "unknown arg: $1"; exit 1 ;;
  esac
done

if [[ "$GATE" -eq 21 ]]; then
  echo "set canonical gate=21 (alias gate=2.1)"
else
  echo "set canonical gate=${GATE}"
fi
