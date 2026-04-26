#!/data/data/com.termux/files/usr/bin/bash

echo "=== Detailed Frame Processing Timing Test ==="
echo

# Enable SoftIsp logging to see timing information
export LD_LIBRARY_PATH=build/src
export LIBCAMERA_LOG_LEVELS="SoftIsp:Debug,SoftISPPipeline:Debug,*:Error"

echo "Capturing frames with detailed timing information..."
echo

# Try to capture frames and show timing
build/src/apps/cam/cam -c 0 --capture=5 --file=/dev/null 2>&1 |& grep -E "(processStats|computeParams|completed in|SoftIsp)" &

# Give it a moment to run
sleep 10

echo
echo "=== Frame Processing Timing Analysis ==="
echo "Process Stats Timing:"
timeout 15s build/src/apps/cam/cam -c 0 --capture=3 --file=/dev/null 2>&1 |& grep -E "processStats completed in" | head -5

echo
echo "Compute Params Timing:"
timeout 15s build/src/apps/cam/cam -c 0 --capture=3 --file=/dev/null 2>&1 |& grep -E "computeParams completed in" | head -5

echo
echo "=== Timing Test Complete ==="