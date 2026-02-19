#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

PORT="${1:-/dev/cu.usbmodem1101}"

export IDF_PYTHON_ENV_PATH="${IDF_PYTHON_ENV_PATH:-/Users/arif/.espressif/python_env/idf5.5_py3.14_env}"
source "${IDF_PATH:-/Users/arif/esp/esp-idf}/export.sh"

idf.py -C "$REPO_ROOT/firmware" build
idf.py -C "$REPO_ROOT/firmware" -p "$PORT" flash monitor
