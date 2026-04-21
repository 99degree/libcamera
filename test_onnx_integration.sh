#!/bin/bash
# Test script to verify ONNX integration in SoftISP

echo "=== SoftISP ONNX Integration Test ==="
echo ""

# Check if ONNX Runtime is available
echo "1. Checking ONNX Runtime..."
if [ -f /data/data/com.termux/files/usr/lib/libonnxruntime.so ]; then
    echo "   ✓ ONNX Runtime library found"
else
    echo "   ✗ ONNX Runtime library not found"
    exit 1
fi

# Check if ONNX models exist
echo ""
echo "2. Checking ONNX models..."
if [ -f "/data/data/com.termux/files/home/softisp_models/algo.onnx" ]; then
    echo "   ✓ algo.onnx found"
else
    echo "   ✗ algo.onnx not found"
    exit 1
fi

if [ -f "/data/data/com.termux/files/home/softisp_models/applier.onnx" ]; then
    echo "   ✓ applier.onnx found"
else
    echo "   ✗ applier.onnx not found"
    exit 1
fi

# Check if IPA modules are built
echo ""
echo "3. Checking IPA modules..."
if [ -f "build/src/ipa/softisp/ipa_softisp.so" ]; then
    echo "   ✓ ipa_softisp.so built"
    ls -lh build/src/ipa/softisp/ipa_softisp.so
else
    echo "   ✗ ipa_softisp.so not built"
    exit 1
fi

if [ -f "build/src/ipa/softisp/ipa_softisp_virtual.so" ]; then
    echo "   ✓ ipa_softisp_virtual.so built"
    ls -lh build/src/ipa/softisp/ipa_softisp_virtual.so
else
    echo "   ✗ ipa_softisp_virtual.so not built"
    exit 1
fi

# Check if ipaCreate is exported
echo ""
echo "4. Checking IPA module exports..."
if nm -D build/src/ipa/softisp/ipa_softisp.so | grep -q "ipaCreate"; then
    echo "   ✓ ipaCreate function exported"
else
    echo "   ✗ ipaCreate function not exported"
    exit 1
fi

# Check if init and processStats are implemented
echo ""
echo "5. Checking SoftIsp methods..."
if nm -D build/src/ipa/softisp/ipa_softisp.so | grep -q "SoftIsp4init"; then
    echo "   ✓ init() method implemented"
else
    echo "   ✗ init() method not implemented"
    exit 1
fi

if nm -D build/src/ipa/softisp/ipa_softisp.so | grep -q "SoftIsp12processStats"; then
    echo "   ✓ processStats() method implemented"
else
    echo "   ✗ processStats() method not implemented"
    exit 1
fi

echo ""
echo "=== All checks passed! ==="
echo ""
echo "To test with a camera, set the environment variable:"
echo "  export SOFTISP_MODEL_DIR=/data/data/com.termux/files/home/softisp_models"
echo ""
echo "Then run libcamera applications with the softisp pipeline."
