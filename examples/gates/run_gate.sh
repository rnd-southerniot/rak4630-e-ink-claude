#!/usr/bin/env bash
# Build, flash, and monitor the currently selected gate on the RAK4630 (nRF52840)
# using PlatformIO. Gate selection is done by set_gate_new.sh (edits pio/platformio.ini).
#
# Usage: run_gate.sh [serial_port]
#   serial_port  optional; PlatformIO auto-detects if omitted (e.g. /dev/cu.usbmodem1101)
#
# Override the pio binary with PIO=/path/to/pio if it is not at the default location.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

PORT="${1:-}"
PIO="${PIO:-$HOME/.platformio/penv/bin/pio}"

if [[ ! -x "$PIO" ]]; then
  echo "error: pio not found at $PIO (set PIO=/path/to/pio)" >&2
  exit 1
fi

cd "$REPO_ROOT/pio"

if [[ -n "$PORT" ]]; then
  "$PIO" run -t upload -t monitor --upload-port "$PORT" --monitor-port "$PORT"
else
  "$PIO" run -t upload -t monitor
fi
