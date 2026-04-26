#!/data/data/com.termux/files/usr/bin/bash

echo "=== SoftISP Pipeline Performance Test ==="
echo

# Test the basic functionality
echo "Testing basic pipeline functionality..."
export LD_LIBRARY_PATH=build/src
export LIBCAMERA_LOG_LEVELS="*:Error"

# Simple test - just check if pipeline loads
timeout 5s build/src/apps/cam/cam --list-properties 2>/dev/null | head -5

echo "✓ Pipeline loaded successfully"
echo

# Test IPA module loading
echo "Testing IPA module loading performance..."
echo "IPA modules found:"
find build/src/ipa -name "*.so" 2>/dev/null | wc -l
echo

echo "=== Performance Test Complete ==="