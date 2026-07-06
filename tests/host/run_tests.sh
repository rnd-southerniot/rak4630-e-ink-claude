#!/usr/bin/env bash
# run_tests.sh — build and run the host-native unit tests via CMake + ctest.
# Targets the ESP-IDF firmware/ tree (canonical after the pio/ consolidation). Same path CI uses.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cmake -S "$HERE" -B "$HERE/build" -G Ninja
cmake --build "$HERE/build"
ctest --test-dir "$HERE/build" --output-on-failure
