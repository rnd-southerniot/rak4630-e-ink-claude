#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 <legacy_gate:0..8>"
  exit 1
fi

LEGACY="$1"
if ! [[ "$LEGACY" =~ ^[0-9]$ ]] || [[ "$LEGACY" -gt 8 ]]; then
  echo "invalid legacy gate: $LEGACY (allowed 0..8)"
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

set_value CONFIG_APP_GATE_LEGACY "$LEGACY"

if rg -q '^# CONFIG_APP_GATE_ID_SCHEME_LEGACY is not set$' "$SDK"; then
  perl -pi -e 's/^# CONFIG_APP_GATE_ID_SCHEME_LEGACY is not set$/CONFIG_APP_GATE_ID_SCHEME_LEGACY=y/' "$SDK"
elif ! rg -q '^CONFIG_APP_GATE_ID_SCHEME_LEGACY=y$' "$SDK"; then
  echo 'CONFIG_APP_GATE_ID_SCHEME_LEGACY=y' >> "$SDK"
fi

if rg -q '^CONFIG_APP_GATE_ID_SCHEME_NEW=y$' "$SDK"; then
  perl -pi -e 's/^CONFIG_APP_GATE_ID_SCHEME_NEW=y/# CONFIG_APP_GATE_ID_SCHEME_NEW is not set/' "$SDK"
elif ! rg -q '^# CONFIG_APP_GATE_ID_SCHEME_NEW is not set$' "$SDK"; then
  echo '# CONFIG_APP_GATE_ID_SCHEME_NEW is not set' >> "$SDK"
fi

echo "set legacy gate=${LEGACY}"
