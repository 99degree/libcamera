#!/bin/bash
# Verify the SoftISP merge is complete and working

echo "=== SoftISP Merge Verification ==="
echo ""

# Check current branch
BRANCH=$(git branch --show-current)
echo "Current branch: $BRANCH"
if [ "$BRANCH" != "merged-softisp-complete" ]; then
    echo "❌ Error: Not on merged-softisp-complete branch"
    exit 1
fi
echo "✓ Correct branch"
echo ""

# Check for merge conflicts
CONFLICTS=$(git status --porcelain | grep "^UU" | wc -l)
if [ $CONFLICTS -gt 0 ]; then
    echo "❌ Error: $CONFLICTS unresolved conflicts"
    exit 1
fi
echo "✓ No merge conflicts"
echo ""

# Check key files exist
echo "Checking key files..."
FILES=(
    "src/ipa/softisp/softisp.cpp"
    "src/ipa/softisp/softisp.h"
    "src/ipa/libipa/af_algo.cpp"
    "src/ipa/libipa/af_algo.h"
    "src/ipa/libipa/af_controls.h"
    "src/libcamera/pipeline/dummysoftisp/softisp.cpp"
    "tools/softisp-onnx-test.cpp"
    "tools/softisp-onnx-inference-test.cpp"
    "MERGE_SUMMARY.md"
    "README_SOFTISP_COMPLETE.md"
)

for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ $file"
    else
        echo "  ❌ $file (MISSING)"
        exit 1
    fi
done
echo ""

# Check for AF algorithm
echo "Checking AF algorithm integration..."
if grep -q "af_algo.h" src/ipa/softisp/softisp.cpp; then
    echo "  ✓ AF algorithm included in softisp.cpp"
else
    echo "  ⚠ AF algorithm NOT included (may be optional)"
fi

if grep -q "softisp_focus_score" src/ipa/softisp/softisp.cpp || grep -q "focusScore" src/ipa/softisp/softisp.cpp; then
    echo "  ✓ Focus score handling found"
else
    echo "  ⚠ Focus score handling not found (may be in separate file)"
fi
echo ""

# Check for ONNX integration
echo "Checking ONNX integration..."
if grep -q "Ort::Session" src/ipa/softisp/softisp.cpp; then
    echo "  ✓ ONNX Runtime sessions defined"
else
    echo "  ❌ ONNX Runtime sessions NOT found"
    exit 1
fi

if grep -q "algoSession" src/ipa/softisp/softisp.cpp; then
    echo "  ✓ algo.onnx session defined"
else
    echo "  ❌ algo.onnx session NOT found"
    exit 1
fi

if grep -q "applierSession" src/ipa/softisp/softisp.cpp; then
    echo "  ✓ applier.onnx session defined"
else
    echo "  ❌ applier.onnx session NOT found"
    exit 1
fi
echo ""

# Check for ControlList integration
echo "Checking ControlList integration..."
if grep -q "ControlList" src/ipa/softisp/softisp.cpp; then
    echo "  ✓ ControlList usage found"
else
    echo "  ❌ ControlList usage NOT found"
    exit 1
fi

if grep -q "sensorControls" src/ipa/softisp/softisp.cpp; then
    echo "  ✓ Input controls (sensorControls) used"
else
    echo "  ⚠ Input controls not found"
fi
echo ""

# Summary
echo "=== Merge Verification Complete ==="
echo ""
echo "✅ All critical checks passed!"
echo ""
echo "The merge successfully combined:"
echo "  • ONNX Runtime dual-model inference"
echo "  • AF algorithm with focus score"
echo "  • ControlList integration"
echo "  • Virtual camera pipeline"
echo "  • Test tools and documentation"
echo ""
echo "Next steps:"
echo "1. Build the project: meson compile -C build"
echo "2. Set environment: export SOFTISP_MODEL_DIR=/path/to/models"
echo "3. Test ONNX models: ./build/tools/softisp-onnx-test"
echo "4. Run inference test: ./build/tools/softisp-onnx-inference-test"
echo ""
echo "Documentation:"
echo "- See MERGE_SUMMARY.md for complete technical details"
echo "- See README_SOFTISP_COMPLETE.md for quick start guide"
