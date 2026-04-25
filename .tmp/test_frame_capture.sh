#!/bin/bash
set -e

echo "=== SoftISP Frame Capture Test ==="
echo ""

# Setup environment
export SOFTISP_MODEL_DIR="./models"
export LD_LIBRARY_PATH="./build/src/ipa/softisp:./build/src/libcamera:$LD_LIBRARY_PATH"

echo "1. Verifying models exist..."
if [ ! -f "./models/algo.onnx" ] || [ ! -f "./models/applier.onnx" ]; then
    echo "❌ ERROR: Models not found in ./models/"
    exit 1
fi
echo "   ✅ algo.onnx: $(ls -lh ./models/algo.onnx | awk '{print $5}')"
echo "   ✅ applier.onnx: $(ls -lh ./models/applier.onnx | awk '{print $5}')"

echo ""
echo "2. Listing available cameras..."
./build/src/apps/cam/cam --list 2>&1 | grep -E "softisp_virtual|Available" || {
    echo "❌ ERROR: Virtual camera not found"
    exit 1
}

echo ""
echo "3. Attempting frame capture (1 frame)..."
OUTPUT_FILE="./test_output_frame.bin"
rm -f "$OUTPUT_FILE"

# Try capture with verbose logging
timeout 15 ./build/src/apps/cam/cam \
    -c 0 \
    -C=1 \
    -F="$OUTPUT_FILE" \
    -v 2>&1 | tee .tmp/capture_log.txt | grep -E "SoftISP|IPA|Error|Frame|Request|buffer|started|stopped|Writing" || true

echo ""
echo "4. Checking output..."
if [ -f "$OUTPUT_FILE" ]; then
    SIZE=$(ls -lh "$OUTPUT_FILE" | awk '{print $5}')
    echo "   ✅ Frame captured successfully!"
    echo "   📁 File: $OUTPUT_FILE"
    echo "   📏 Size: $SIZE"
    
    # Show first few bytes as hex
    echo "   🔍 First 64 bytes (hex):"
    xxd -l 64 "$OUTPUT_FILE" 2>/dev/null || hexdump -C -n 64 "$OUTPUT_FILE" 2>/dev/null || echo "      (unable to display hex)"
else
    echo "   ⚠️  Frame file not created"
    echo ""
    echo "   Checking logs for errors..."
    grep -i "error\|fail\|exception" .tmp/capture_log.txt | head -10 || echo "   No obvious errors found in log"
fi

echo ""
echo "5. Analysis complete."
