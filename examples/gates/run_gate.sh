#!/usr/bin/env bash
# Build, flash, and monitor the currently selected gate using PlatformIO.
# Gate selection is done by set_gate_new.sh (edits pio/platformio.ini build flags).
#
# Usage: run_gate.sh [serial_port] [env]
#   serial_port  optional; PlatformIO auto-detects if omitted (e.g. /dev/cu.usbmodem1101)
#   env          PlatformIO env: rak4631 (default, nRF52840) or rak3312 (ESP32-S3)
#
# Override the pio binary with PIO=/path/to/pio if it is not at the default location.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

PORT="${1:-}"
ENV="${2:-${PIO_ENV:-rak4631}}"
PIO="${PIO:-$HOME/.platformio/penv/bin/pio}"

if [[ ! -x "$PIO" ]]; then
  echo "error: pio not found at $PIO (set PIO=/path/to/pio)" >&2
  exit 1
fi

cd "$REPO_ROOT/pio"

if [[ -n "$PORT" ]]; then
  "$PIO" run -e "$ENV" -t upload -t monitor --upload-port "$PORT" --monitor-port "$PORT"
else
  "$PIO" run -e "$ENV" -t upload -t monitor
fi
