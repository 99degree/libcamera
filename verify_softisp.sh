#!/bin/bash
# SoftISP Verification Script
# This script verifies the core SoftISP components are built correctly

echo "========================================"
echo "SoftISP Verification Script"
echo "========================================"
echo

# Check IPA modules
echo "1. Checking IPA modules..."
if [ -f "build/src/ipa/softisp/ipa_softisp.so" ]; then
    echo "   ✓ ipa_softisp.so exists ($(ls -lh build/src/ipa/softisp/ipa_softisp.so | awk '{print $5}'))"
    
    # Check symbols
    if nm -D build/src/ipa/softisp/ipa_softisp.so | grep -q "ipaModuleInfo"; then
        echo "   ✓ ipaModuleInfo symbol exported"
    else
        echo "   ✗ ipaModuleInfo symbol NOT found"
        exit 1
    fi
    
    if nm -D build/src/ipa/softisp/ipa_softisp.so | grep -q "ipaCreate"; then
        echo "   ✓ ipaCreate symbol exported"
    else
        echo "   ✗ ipaCreate symbol NOT found"
        exit 1
    fi
else
    echo "   ✗ ipa_softisp.so NOT found"
    exit 1
fi

if [ -f "build/src/ipa/softisp/ipa_softisp_virtual.so" ]; then
    echo "   ✓ ipa_softisp_virtual.so exists ($(ls -lh build/src/ipa/softisp/ipa_softisp_virtual.so | awk '{print $5}'))"
    
    if nm -D build/src/ipa/softisp/ipa_softisp_virtual.so | grep -q "ipaModuleInfo"; then
        echo "   ✓ ipaModuleInfo symbol exported (virtual)"
    else
        echo "   ✗ ipaModuleInfo symbol NOT found (virtual)"
        exit 1
    fi
else
    echo "   ✗ ipa_softisp_virtual.so NOT found"
    exit 1
fi

echo

# Check pipeline handlers
echo "2. Checking pipeline handlers..."
if [ -f "build/src/libcamera/libcamera.so.p/pipeline_softisp_softisp.cpp.o" ]; then
    echo "   ✓ softisp pipeline handler compiled"
else
    echo "   ✗ softisp pipeline handler NOT compiled"
    exit 1
fi

if [ -f "build/src/libcamera/libcamera.so.p/pipeline_dummysoftisp_softisp.cpp.o" ]; then
    echo "   ✓ dummysoftisp pipeline handler compiled"
else
    echo "   ✗ dummysoftisp pipeline handler NOT compiled"
    exit 1
fi

echo

# Check test application
echo "3. Checking test application..."
if [ -f "build/tools/softisp-test-app" ]; then
    echo "   ✓ softisp-test-app exists"
    echo "   Usage: ./build/tools/softisp-test-app --help"
else
    echo "   ✗ softisp-test-app NOT found"
    exit 1
fi

echo

# Check ONNX models
echo "4. Checking ONNX models..."
MODEL_DIR="${SOFTISP_MODEL_DIR:-/data/data/com.termux/files/home/softisp_models}"
if [ -d "$MODEL_DIR" ]; then
    if [ -f "$MODEL_DIR/algo.onnx" ] && [ -f "$MODEL_DIR/applier.onnx" ]; then
        echo "   ✓ ONNX models found in $MODEL_DIR"
        echo "     - algo.onnx ($(ls -lh $MODEL_DIR/algo.onnx | awk '{print $5}'))"
        echo "     - applier.onnx ($(ls -lh $MODEL_DIR/applier.onnx | awk '{print $5}'))"
    else
        echo "   ⚠ ONNX models directory exists but models not found"
        echo "     Expected: $MODEL_DIR/algo.onnx and $MODEL_DIR/applier.onnx"
    fi
else
    echo "   ⚠ ONNX models directory not found: $MODEL_DIR"
    echo "     Set SOFTISP_MODEL_DIR to the directory containing .onnx files"
fi

echo

# Summary
echo "========================================"
echo "Verification Summary"
echo "========================================"
echo "✓ All core SoftISP components are built correctly"
echo "✓ IPA modules export required symbols"
echo "✓ Pipeline handlers are compiled"
echo "✓ Test application is available"
echo
echo "Note: Full end-to-end testing requires:"
echo "  - A V4L2 camera device (for softisp pipeline)"
echo "  - Or a media controller device (for dummysoftisp pipeline)"
echo "  - ONNX model files in SOFTISP_MODEL_DIR"
echo
echo "To test with real hardware:"
echo "  export SOFTISP_MODEL_DIR=/path/to/models"
echo "  ./build/tools/softisp-test-app --pipeline softisp"
echo
echo "========================================"
