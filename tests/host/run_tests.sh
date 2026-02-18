#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="$ROOT_DIR/tests/host/.build"
mkdir -p "$BUILD_DIR"

cc -std=c11 -Wall -Wextra -Werror \
  "$ROOT_DIR/tests/host/payload_encode_test.c" \
  "$ROOT_DIR/firmware/main/app_payload.c" \
  -I"$ROOT_DIR/firmware/main" \
  -lm \
  -o "$BUILD_DIR/payload_encode_test"

cc -std=c11 -Wall -Wextra -Werror \
  "$ROOT_DIR/tests/host/gate_id_map_test.c" \
  "$ROOT_DIR/firmware/main/gate_id_map.c" \
  -I"$ROOT_DIR/firmware/main" \
  -o "$BUILD_DIR/gate_id_map_test"

"$BUILD_DIR/payload_encode_test"
"$BUILD_DIR/gate_id_map_test"
