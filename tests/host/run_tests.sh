#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/tests/host/.build"
mkdir -p "$BUILD_DIR"

# Tests target the active PlatformIO (nRF52840) tree under pio/, not the legacy
# ESP-IDF firmware/main/ tree.
cc -std=c11 -Wall -Wextra -Werror \
  "$ROOT_DIR/tests/host/payload_encode_test.c" \
  "$ROOT_DIR/pio/src/app_payload.c" \
  -I"$ROOT_DIR/pio/include" \
  -lm \
  -o "$BUILD_DIR/payload_encode_test"

cc -std=c11 -Wall -Wextra -Werror \
  "$ROOT_DIR/tests/host/gate_id_map_test.c" \
  "$ROOT_DIR/pio/src/gate_id_map.c" \
  -I"$ROOT_DIR/pio/include" \
  -o "$BUILD_DIR/gate_id_map_test"

"$BUILD_DIR/payload_encode_test"
"$BUILD_DIR/gate_id_map_test"
