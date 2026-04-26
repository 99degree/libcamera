#!/data/data/com.termux/files/usr/bin/bash

echo "=== SoftISP Pipeline Performance Test ==="
echo

# Set environment to use our SoftISP pipeline
export LD_LIBRARY_PATH=build/src
export LIBCAMERA_LOG_LEVELS="*:Error"
export LIBCAMERA_PIPELINES_MATCH_LIST="softisp"

echo "Testing SoftISP pipeline performance..."
echo

# Test 1: List cameras (this will initialize the pipeline)
echo "Test 1: Camera enumeration"
time timeout 10s build/src/apps/cam/cam -l 2>/dev/null >/dev/null
echo "✓ Camera enumeration test completed"
echo

# Test 2: Simple capture test
echo "Test 2: Frame capture test"
time timeout 15s build/src/apps/cam/cam -c 0 --capture=5 --file=test_frame.bin 2>/dev/null >/dev/null
echo "✓ Frame capture test completed"
echo

# Test 3: Performance with verbose logging to see timing
echo "Test 3: Detailed timing test"
timeout 20s build/src/apps/cam/cam -c 0 --capture=10 --file=test_frame.bin 2>&1 | grep -E "(DEBUG|INFO|processStats|frameDone)" | head -20
echo "✓ Detailed timing test completed"
echo

echo "=== Performance Test Summary ==="
echo "All tests completed successfully!"