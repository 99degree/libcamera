#!/bin/bash
# SoftISP Frame Capture Script
# Captures frames and saves RGB, YUV, and metadata

set -e

CAMERA_ID="${1:-SoftISP}"
FRAMES="${2:-3}"
OUTPUT_DIR="${3:-./frames}"

echo "=== SoftISP Frame Capture ==="
echo "Camera: $CAMERA_ID"
echo "Frames: $FRAMES"
echo "Output: $OUTPUT_DIR"
echo ""

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Set environment variables
export LD_LIBRARY_PATH=build/src/libcamera:build/src/ipa/dummysoftisp:$LD_LIBRARY_PATH
export SOFTISP_MODEL_DIR=/data/data/com.termux/files/home/softisp_models

# Capture frames using the test app
echo "Capturing frames..."
./build/tools/softisp-save -c "$CAMERA_ID" -f "$FRAMES" -o "$OUTPUT_DIR"

echo ""
echo "=== Frame Files ==="
ls -lh "$OUTPUT_DIR"/*.bin 2>/dev/null || echo "No .bin files found"
echo ""
echo "=== Metadata Files ==="
ls -lh "$OUTPUT_DIR"/*.json 2>/dev/null || echo "No .json files found"
echo ""
echo "Done!"
