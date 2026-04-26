#!/data/data/com.termux/files/usr/bin/bash

echo "=== SoftISP Frame Processing Performance Test ==="
echo

# Enable detailed logging to see timing information
export LD_LIBRARY_PATH=build/src
export LIBCAMERA_LOG_LEVELS="SoftIsp:Info,SoftISPPipeline:Info,*:Error"

echo "Testing frame processing timing with detailed logging..."
echo

# Capture a few frames with timing enabled
timeout 30s build/src/apps/cam/cam -c 0 --capture=10 --file=/dev/null 2>&1 | grep -E "(processStats|computeParams|completed in|frame=)" | head -20

echo
echo "=== Frame Processing Timing Test Complete ==="