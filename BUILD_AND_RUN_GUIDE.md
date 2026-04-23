# SoftISP Build and Run Guide

This guide provides step-by-step instructions to build and run the SoftISP pipeline with ONNX-based image processing on Termux (Android) or Linux.

## Prerequisites

### Termux (Android)
```bash
# Update packages
pkg update && pkg upgrade

# Install build tools
pkg install clang ninja meson python3 python-onnxruntime libevent libyaml git

# Install Python dependencies (if not included in pkg)
pip install onnxruntime
```

### Linux (Debian/Ubuntu)
```bash
# Update packages
sudo apt update

# Install build tools
sudo apt install build-essential ninja-build meson python3 python3-pip git

# Install ONNX Runtime development files
sudo apt install libonnxruntime-dev

# Install Python dependencies
pip3 install onnxruntime

# Install other dependencies
sudo apt install libyaml-dev libevent-dev
```

## Build Instructions

### 1. Clone and Checkout
```bash
git clone https://github.com/99degree/libcamera.git
cd libcamera
git checkout softisp_new
```

### 2. Configure Build
```bash
# Configure with SoftISP enabled by default
meson setup build \
  -Dpipelines=softisp \
  -Dsoftisp=enabled \
  -Ddevelopment=true \
  -Dcam=enabled
```

**Configuration Options:**
- `-Dpipelines=softisp`: Set SoftISP as the default pipeline
- `-Dsoftisp=enabled`: Force enable SoftISP (overrides auto-detection)
- `-Ddevelopment=true`: Skip IPA signature verification (required for testing)
- `-Dcam=enabled`: Build the `cam` application for testing

### 3. Compile
```bash
meson compile -C build
```

This will build:
- `build/src/libcamera/libcamera.so` - Main library
- `build/src/ipa/softisp/ipa_softisp.so` - SoftISP IPA module
- `build/src/apps/cam/cam` - Camera application
- `build/softisp-tool` - ONNX testing utility

## Runtime Configuration

### Set Environment Variables
```bash
# Path to build artifacts
export LD_LIBRARY_PATH=/path/to/libcamera/build/src/libcamera:/path/to/libcamera/build/src/ipa/softisp:$LD_LIBRARY_PATH

# Set IPA search paths
export LIBCAMERA_PIPELINES=/path/to/libcamera/build/src/libcamera/pipeline/softisp
export LIBCAMERA_IPA=/path/to/libcamera/build/src/ipa/softisp

# Set model directory (required for ONNX inference)
export SOFTISP_MODEL_DIR=/path/to/libcamera
```

**Note:** Replace `/path/to/libcamera` with your actual build directory.

## Running Tests

### 1. Test ONNX Model Loading
```bash
# Test algo.onnx
./build/softisp-tool test algo.onnx

# Test applier.onnx
./build/softisp-tool test applier.onnx

# Inspect model metadata
./build/softisp-tool inspect algo.onnx
./build/softisp-tool shapes applier.onnx
```

### 2. List Available Cameras
```bash
./build/src/apps/cam/cam -l
```

Expected output:
```
Available cameras:
1: (softisp_virtual)
```

### 3. Run Virtual Camera Test
```bash
# Capture a single frame from virtual camera
./build/src/apps/cam/cam -c "softisp_virtual" --capture=1 --file=test_frame.ppm
```

### 4. Enable Debug Logging
```bash
export LIBCAMERA_LOG_LEVELS="SoftIsp:Info,IPAManager:Info,SoftISPPipeline:Info"
./build/src/apps/cam/cam -l 2>&1 | grep -E "(SoftIsp|initialized|Models loaded)"
```

## Expected Output

### Successful Build
```
[10/10] Linking target src/apps/cam/cam
Build finished successfully.
```

### Successful IPA Initialization
```
[INFO] SoftIsp softisp.cpp:17 SoftIsp created
[INFO] SoftIsp softisp.cpp:20 SoftIsp::init() called
[INFO] SoftIsp softisp.cpp:21 Model dir env: /path/to/libcamera
[INFO] SoftIsp softisp.cpp:68 SoftISP initialized for 1920x1080
[INFO] SoftIsp softisp.cpp:69 Models loaded from: /path/to/libcamera
```

### Successful Camera Capture
```
[INFO] VirtualCamera virtual_camera.cpp:34 Virtual camera initialized: 1920x1080
[INFO] Camera camera_manager.cpp:226 Adding camera 'softisp_virtual'
[INFO] SoftISPPipeline softisp.cpp:236 Virtual camera registered successfully
Frame captured successfully: test_frame.ppm
```

## Troubleshooting

### Issue: "IPA module not available"
**Cause:** Pipeline name mismatch or version mismatch.
**Solution:**
1. Ensure `pipelineName` in `softisp_module.cpp` is `"SoftISP"` (capital S).
2. Ensure `createIPA()` uses version `1, 1`.
3. Rebuild both the pipeline and IPA module.

### Issue: "SOFTISP_MODEL_DIR environment variable not set"
**Cause:** Model directory not specified.
**Solution:**
```bash
export SOFTISP_MODEL_DIR=/path/to/libcamera
```
Ensure `algo.onnx` and `applier.onnx` exist in that directory.

### Issue: "Failed to load algo model"
**Cause:** ONNX model file missing or corrupted.
**Solution:**
```bash
ls -lh algo.onnx applier.onnx
./build/softisp-tool test algo.onnx
```

### Issue: "libonnxruntime.so: cannot open shared object file"
**Cause:** ONNX Runtime library not found.
**Solution:**
```bash
# Termux
pkg install python-onnxruntime

# Linux
sudo apt install libonnxruntime-dev
```

### Issue: Camera session creation fails
**Cause:** Virtual camera lifecycle or hardware not available.
**Solution:**
- This is expected in test environments without real hardware.
- The virtual camera works for pipeline testing but requires real hardware for full functionality.

## Advanced Usage

### Real Hardware Deployment (Raspberry Pi / Rockchip)
1. **Copy build artifacts to target device:**
   ```bash
   scp build/src/libcamera/libcamera.so user@device:/usr/lib/libcamera/
   scp build/src/ipa/softisp/ipa_softisp.so user@device:/usr/lib/libcamera/ipa/
   scp algo.onnx applier.onnx user@device:/usr/share/libcamera/ipa/softisp/
   ```

2. **Install tuning files** (sensor-specific `.json`/`.yaml`):
   ```bash
   scp sensor_tuning.json user@device:/usr/share/libcamera/ipa/softisp/
   ```

3. **Configure environment on target:**
   ```bash
   export LIBCAMERA_IPA=/usr/lib/libcamera/ipa/softisp
   export SOFTISP_MODEL_DIR=/usr/share/libcamera/ipa/softisp
   ```

4. **Test with real camera:**
   ```bash
   libcamera-hello
   ```

### Build Optimization
```bash
# Release build with optimizations
meson setup build --buildtype=release

# Enable more aggressive optimizations
CFLAGS="-O3 -march=native" meson setup build
```

## Verification Checklist

- [ ] Build completes without errors
- [ ] `libcamera.so` exists in `build/src/libcamera/`
- [ ] `ipa_softisp.so` exists in `build/src/ipa/softisp/`
- [ ] `cam` application runs and lists cameras
- [ ] `softisp-tool` successfully tests ONNX models
- [ ] Virtual camera captures frames
- [ ] Debug logs show "SoftISP initialized" and "Models loaded"

## Next Steps

1. **Deploy to real hardware** with a compatible camera sensor.
2. **Configure sensor-specific tuning files** for optimal image quality.
3. **Benchmark performance** and optimize ONNX model inference.
4. **Integrate with applications** using the libcamera API.

---
**Last Updated:** 2026-04-24  
**Branch:** `softisp_new`  
**Commit:** Latest  
**Environment:** Termux (aarch64) / Linux
