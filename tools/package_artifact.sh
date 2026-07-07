#!/usr/bin/env bash
# package_artifact.sh — assemble the senseflow firmware artifact the CRM firmware-service serves.
#
# P5.1 cross-repo sourcing is "model B" (pre-built artifact): senseflow publishes a tagged bundle and
# the careflow firmware-service references it by product+tag (unpacked under its SENSEFLOW_ROOT). This
# script assembles that bundle from a completed ESP-IDF build + the device-profiles catalog, mirroring
# the layout the service's product registry expects:
#
#   <out>/senseflow_eink_node.bin      app image           (build/senseflow_eink_node.bin)
#   <out>/bootloader.bin               boot part            (build/bootloader/bootloader.bin)
#   <out>/partition-table.bin          boot part            (build/partition_table/partition-table.bin)
#   <out>/ota_data.bin                 boot part            (build/ota_data_initial.bin, renamed)
#   <out>/compiled_sensors.json        flashable manifest   (device-profiles/, gen_compiled_sensors.py)
#   <out>/device-profiles/*.json       CRM catalog (blobHex + i2c metadata; profiles_to_catalog.py)
#   <out>/ARTIFACT.json                {product, tag, builtAt, sha256 of each part}
#
# The bundle is then uploaded (by the CRM team / Fahim) to MinIO builds/senseflow/<tag>/ and unpacked
# under the service's SENSEFLOW_ROOT. No senseflow toolchain runs inside the service.
#
# Usage:
#   tools/package_artifact.sh <firmware-tag> [out-dir]
#   FIRMWARE_TAG=senseflow-p4-green tools/package_artifact.sh   # tag via env
#
# Assumes `idf.py -C firmware build` has already run (does NOT build — build is a separate, gated step).
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$REPO/firmware/build"
PROFILES="$REPO/device-profiles"
BIN="senseflow_eink_node.bin"

TAG="${1:-${FIRMWARE_TAG:-}}"
OUT="${2:-$REPO/dist/senseflow/${TAG:-untagged}}"

if [[ -z "$TAG" ]]; then
    echo "ERROR: firmware tag required (arg 1 or \$FIRMWARE_TAG)" >&2
    echo "usage: $0 <firmware-tag> [out-dir]" >&2
    exit 2
fi

# --- Preconditions: a completed build + fresh manifests -----------------------------------------
require() { test -f "$1" || { echo "ERROR: missing $2 ($1) — run 'idf.py -C firmware build' first" >&2; exit 1; }; }
require "$BUILD/$BIN"                                   "app image"
require "$BUILD/bootloader/bootloader.bin"             "bootloader"
require "$BUILD/partition_table/partition-table.bin"   "partition-table"
require "$BUILD/ota_data_initial.bin"                  "ota-data"

# Keep the served manifest + catalog generated-fresh from source (fail loud if stale).
python3 "$PROFILES/gen_compiled_sensors.py" --check
python3 "$PROFILES/profiles_to_catalog.py" --check

# --- Assemble ------------------------------------------------------------------------------------
rm -rf "$OUT"
mkdir -p "$OUT/device-profiles"

cp "$BUILD/$BIN"                                 "$OUT/$BIN"
cp "$BUILD/bootloader/bootloader.bin"            "$OUT/bootloader.bin"
cp "$BUILD/partition_table/partition-table.bin"  "$OUT/partition-table.bin"
cp "$BUILD/ota_data_initial.bin"                 "$OUT/ota_data.bin"          # renamed for the service
cp "$PROFILES/compiled_sensors.json"             "$OUT/compiled_sensors.json"
# Catalog entries only (top-level device-profiles/*.json), NOT the source profiles/, generators, or
# the compiled_sensors.json manifest — the service globs catalog_dir/*.json expecting ONLY catalog
# entries (each has profileKey/blobHex), so anything else there crashes load_sensors_v2.
find "$PROFILES" -maxdepth 1 -name '*.json' ! -name 'compiled_sensors.json' \
    -exec cp {} "$OUT/device-profiles/" \;

# --- Manifest (sha256 per part; deterministic, no build-time secrets) ----------------------------
sha() { shasum -a 256 "$1" | awk '{print $1}'; }
cat > "$OUT/ARTIFACT.json" <<EOF
{
  "product": "senseflow",
  "firmwareTag": "$TAG",
  "binary": "$BIN",
  "parts": {
    "$BIN": "$(sha "$OUT/$BIN")",
    "bootloader.bin": "$(sha "$OUT/bootloader.bin")",
    "partition-table.bin": "$(sha "$OUT/partition-table.bin")",
    "ota_data.bin": "$(sha "$OUT/ota_data.bin")"
  }
}
EOF

echo "Packaged senseflow artifact → $OUT"
echo "  tag:    $TAG"
echo "  app:    $BIN ($(sha "$OUT/$BIN" | cut -c1-16)…)"
echo "  catalog: $(find "$OUT/device-profiles" -name '*.json' | wc -l | tr -d ' ') profile(s), sensors: $(cat "$OUT/compiled_sensors.json" | tr -d '\n[] "')"
echo
echo "Next (CRM team / Fahim): upload $OUT/ to MinIO builds/senseflow/$TAG/ and set SENSEFLOW_ROOT."
