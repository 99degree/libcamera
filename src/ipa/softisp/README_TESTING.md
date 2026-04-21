# SoftISP Testing Guide

## Overview
This document describes how to test the SoftISP IPA module with the ONNX models.

## Prerequisites
- ONNX Runtime installed (`pkg install onnxruntime`)
- SoftISP model files:
  - `algo.onnx` (25KB)
  - `applier.onnx` (20KB)
- Place models in: `/data/data/com.termux/files/home/softisp_models/`

## Quick Test: Model Validation

### Build the test program
```bash
cd /data/data/com.termux/files/home/libcamera
meson setup build -Dsoftisp=enabled -Dtest=true -Dc_args=-Wno-error -Dcpp_args=-Wno-error
meson compile -C build softisp_test
```

### Run the test
```bash
./build/src/ipa/softisp/softisp_test
```

**Expected output:**
```
Loading algo.onnx from: /data/data/com.termux/files/home/softisp_models/algo.onnx
algo.onnx loaded successfully!
Loading applier.onnx from: /data/data/com.termux/files/home/softisp_models/applier.onnx
applier.onnx loaded successfully!
algo.onnx: 4 inputs, 15 outputs
applier.onnx: 10 inputs, 7 outputs
SoftISP models loaded and ready for inference!
```

## Full Inference Test

### Build the full test program
```bash
meson compile -C build softisp_full_test
```

### Run the full test
```bash
./build/src/ipa/softisp/softisp_full_test
```

This test runs the complete two-stage inference pipeline with dummy data and extracts AWB gains.

## Testing with Real Camera (Once Build Issues are Resolved)

### Build libcamera with SoftISP
```bash
meson setup build -Dsoftisp=enabled -Dpipelines=simple
meson compile -C build
```

### Install the SoftISP IPA module
```bash
sudo meson install -C build
```

### Test with libcamera-vid
```bash
export SOFTISP_MODEL_DIR=/data/data/com.termux/files/home/softisp_models
libcamera-vid --pipelines simple --timeout 5000 -o test.264
```

### Test with libcamera-avg
```bash
export SOFTISP_MODEL_DIR=/data/data/com.termux/files/home/softisp_models
libcamera-avg --pipelines simple
```

### Enable debug logging
```bash
export LIBCAMERA_LOG_LEVELS=*:Warn,Softisp:Debug
libcamera-vid --pipelines simple --timeout 5000 -o test.264
```

## Known Issues

### Build Issues on Termux
The full libcamera build may fail on Termux due to:
1. Missing `PTHREAD_MUTEX_ROBUST` in libpisp
2. Missing `pthread_setaffinity_np` in libcamera base
3. nlohmann_json deprecation warnings

These are platform-specific issues unrelated to SoftISP. Workarounds:
- Use the standalone test programs (`softisp_test`, `softisp_full_test`)
- Patch the libpisp and libcamera base code to fix pthread issues
- Use a different platform (e.g., Raspberry Pi, x86 Linux) for full testing

## Model Specifications

### algo.onnx
- **Purpose:** Generate ISP coefficients from AWB statistics
- **Inputs:** 4 (AWB statistics, camera parameters, etc.)
- **Outputs:** 15 (ISP coefficients)

### applier.onnx
- **Purpose:** Apply ISP coefficients to full-resolution image
- **Inputs:** 10 (Full-res Bayer buffer + ISP coefficients)
- **Outputs:** 7 (Final ISP controls including AWB gains)

## Verification Checklist

- [ ] Models load successfully
- [ ] algo.onnx has 4 inputs and 15 outputs
- [ ] applier.onnx has 10 inputs and 7 outputs
- [ ] Two-stage inference completes without errors
- [ ] AWB gains are extracted correctly (R, G, B values)
- [ ] libcamera-vid/libcamera-avg runs with SoftISP enabled
- [ ] Video output shows correct color balance

## Troubleshooting

### Model loading fails
- Check that `SOFTISP_MODEL_DIR` environment variable is set correctly
- Verify model files exist and are not corrupted
- Check ONNX Runtime version compatibility

### Inference fails
- Verify input tensor shapes match model expectations
- Check that all required inputs are provided
- Review ONNX Runtime error messages

### No video output
- Ensure camera hardware is connected and working
- Check that the "simple" pipeline is mapped to SoftISP
- Verify IPA module is installed in the correct directory
