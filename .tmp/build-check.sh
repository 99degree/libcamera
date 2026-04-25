#!/bin/bash
echo "=== Building SoftISP Pipeline ==="
ninja -C build -j4 2>&1 | tee .tmp/build.log

echo ""
echo "=== Checking for Errors ==="
if grep -q "error:" .tmp/build.log; then
    echo "Compilation FAILED. Errors found:"
    grep "error:" .tmp/build.log | head -10
    echo ""
    echo "=== Attempting to fix common issues ==="
    # Show the problematic lines
    grep -B 2 "error:" .tmp/build.log | grep "softisp.cpp" | head -5
else
    echo "Compilation SUCCESSFUL!"
    echo ""
    echo "=== Testing with cam --list ==="
    timeout 3 ./build/src/apps/cam/cam --list 2>&1 | grep -E "SoftISPPipeline|softisp_virtual|Available cameras|Camera [0-9]|Found" | head -20
fi
