#!/usr/bin/env bash
# Select the active gate (and per-gate expected-device counts) for the RAK4630
# PlatformIO build by rewriting the -D build flags in pio/platformio.ini.
#
# Usage:
#   set_gate_new.sh <gate:0..9|2.1|21> \
#       [--gate2p1-devices 1|2|3] [--gate3-devices 1|2|3] \
#       [--gate4-devices 1|2|3]   [--gate9-devices 1|2|3]
#
# Device modes: 1 = SGP40 only, 2 = BMP280 only, 3 = both.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

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

INI="$REPO_ROOT/pio/platformio.ini"

if [[ ! -f "$INI" ]]; then
  echo "error: $INI not found" >&2
  exit 1
fi

# Rewrite an existing "    -D<KEY>=<value>" build flag in place, preserving indentation.
set_flag() {
  local key="$1"
  local value="$2"
  if grep -qE "^[[:space:]]*-D${key}=" "$INI"; then
    perl -pi -e "s/^(\s*)-D${key}=.*/\${1}-D${key}=${value}/" "$INI"
  else
    echo "warning: -D${key} not present in platformio.ini (skipped)" >&2
  fi
}

set_flag APP_GATE "$GATE"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --gate2p1-devices)
      set_flag APP_GATE2P1_EXPECTED_DEVICES "$2"; shift 2 ;;
    --gate3-devices)
      set_flag APP_GATE3_EXPECTED_DEVICES "$2"; shift 2 ;;
    --gate4-devices)
      set_flag APP_GATE4_EXPECTED_DEVICES "$2"; shift 2 ;;
    --gate9-devices)
      set_flag APP_GATE9_EXPECTED_DEVICES "$2"; shift 2 ;;
    *)
      echo "unknown arg: $1"; exit 1 ;;
  esac
done

if [[ "$GATE" -eq 21 ]]; then
  echo "set APP_GATE=21 (alias gate 2.1) in platformio.ini"
else
  echo "set APP_GATE=${GATE} in platformio.ini"
fi
