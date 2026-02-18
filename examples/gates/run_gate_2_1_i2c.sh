#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "usage: $0 <serial_port> [devices:1|2|3]"
  exit 1
fi

PORT="$1"
DEVICES="${2:-3}"

/Users/arif/rak4630-e-ink/examples/gates/set_gate_new.sh 2.1 --gate2p1-devices "$DEVICES"
/Users/arif/rak4630-e-ink/examples/gates/run_gate.sh "$PORT"
