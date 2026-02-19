#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <serial_port> [devices:1|2|3]"
  exit 1
fi

PORT="$1"
DEVICES="${2:-1}"

"$SCRIPT_DIR/set_gate_new.sh" 4 --gate4-devices "$DEVICES"
"$SCRIPT_DIR/run_gate.sh" "$PORT"
