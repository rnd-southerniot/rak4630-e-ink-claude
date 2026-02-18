#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/cu.usbmodem1101}"

export IDF_PYTHON_ENV_PATH="${IDF_PYTHON_ENV_PATH:-/Users/arif/.espressif/python_env/idf5.5_py3.14_env}"
source /Users/arif/esp/esp-idf/export.sh

idf.py -C /Users/arif/rak4630-e-ink/firmware build
idf.py -C /Users/arif/rak4630-e-ink/firmware -p "$PORT" flash monitor
