# Quick Start - SoftISP Full Implementation

## Branch
```bash
git checkout feature/softisp-full-inference
```

## Build
```bash
cd build
meson compile
```

## Test
```bash
# Dummy pipeline (no camera needed)
./build/tools/softisp-test-app --pipeline dummysoftisp --frames 10

# Real camera pipeline (requires /dev/video0)
./build/tools/softisp-test-app --pipeline softisp --frames 10
```

## What Changed

### 3 Commits
1. **processFrame implementation** - IPA side
2. **Pipeline handlers** - dummysoftisp & softisp
3. **Documentation** - Complete summary

### Key Files
- `include/libcamera/ipa/softisp.mojom` - Interface
- `src/ipa/softisp/softisp.cpp` - IPA implementation
- `src/libcamera/pipeline/dummysoftisp/softisp.cpp` - Dummy pipeline
- `src/libcamera/pipeline/softisp/softisp.cpp` - Real camera pipeline

## Data Flow
```
Request → Map Buffer → processStats → processFrame → Complete Request
```

## Next Steps
1. Replace simulated data in `processStats` with real buffer reading
2. Implement real AF focus score calculation
3. Add proper pixel format support (Bayer10, NV12, etc.)
4. Test with actual ONNX models
