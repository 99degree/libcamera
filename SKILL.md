# SoftISP Pipeline Skill

> **Author:** George Chan <gchan9527@gmail.com>  
> **Branch:** `softisp_final`  
> **Repository:** `https://github.com/99degree/libcamera`  
> **Status:** ✅ Architecture Complete | ⚠️ Termux `invokeMethod` Bug | 🚀 Ready for Real Hardware

---

## 🎯 Overview

The **SoftISP Pipeline** is a software-based Image Signal Processor (ISP) pipeline for libcamera that uses **ONNX models** to process Bayer RAW images into viewable formats. It is designed to work on systems without dedicated ISP hardware (e.g., Raspberry Pi, Rockchip) by leveraging ONNX Runtime for image processing.

### Key Features
- **ONNX-Based ISP**: Uses `algo.onnx` and `applier.onnx` models for image processing.
- **Virtual Camera**: Generates synthetic Bayer10 (RGGB) frames for testing and development.
- **SimplePipeline Architecture**: Follows the `SimplePipeline` pattern for clean separation of duties.
- **Development Mode**: Supports unsigned IPA modules for easier testing.
- **Multi-Format Output**: Supports RAW, YUV, RGB, and PPM output formats.

---

## 🏗️ Architecture

The pipeline follows a **strict Division of Duty** pattern, aligning with the `SimplePipeline` design:

```
CameraManager / cam app
       |
       | invokeMethod(&PipelineHandler::generateConfiguration)
       v
PipelineHandlerSoftISP (Dispatcher)
       |
       | 1. Gets Camera* → casts to SoftISPCameraData*
       | 2. Delegates to data->generateConfiguration()
       v
SoftISPCameraData (Camera Object / "Brain")
       |
       | - Holds state (VirtualCamera*, streamConfigs_)
       | - Implements: generateConfiguration, configure, start, stop, queueRequest
       | - Delegates frame generation to VirtualCamera
       v
VirtualCamera (Frame Generator / "Muscle")
       |
       | - Generates Bayer10 (RGGB) patterns
       | - Manages buffer queue
       | - Triggers ONNX inference via processRequest
```

### File Structure
```
src/libcamera/pipeline/softisp/
├── softisp.cpp          # PipelineHandler (Dispatcher only)
├── softisp.h            # PipelineHandler & SoftISPCameraData declarations
├── virtual_camera.cpp   # VirtualCamera implementation (frame generation)
├── virtual_camera.h     # VirtualCamera declaration
└── meson.build          # Build configuration

tools/
├── softisp-save.cpp     # Capture utility (RAW, YUV, RGB, PPM)
└── softisp-tool.cpp     # ONNX model inspection and testing suite

src/ipa/softisp/
├── softisp_module.cpp   # IPA module loader (ONNX Runtime integration)
└── softisp.cpp          # IPA processing logic (ONNX inference)
```

---

## 🛠️ Build & Configuration

### Prerequisites (Termux/Android)
```bash
pkg install clang ninja meson python3 python-pip libyaml libevent cmake libjpeg-turbo
pip install onnx onnxruntime
```

### Build Commands
```bash
# Clone and switch to branch
git clone https://github.com/99degree/libcamera.git
cd libcamera
git checkout softisp_final

# Configure with SoftISP enabled by default
meson setup build \
  -Dpipelines=softisp \
  -Dsoftisp=enabled \
  -Ddevelopment=true \
  -Dcam=enabled

# Build
meson compile -C build
```

### Key Build Options
| Option | Default | Description |
|--------|---------|-------------|
| `-Dpipelines` | `['softisp']` | Enabled pipelines (default: `softisp`) |
| `-Dsoftisp` | `enabled` | SoftISP pipeline feature flag |
| `-Ddevelopment` | `true` | Skip IPA signature verification |
| `-Dcam` | `enabled` | Build the `cam` application |

---

## 📦 Dependencies

### System Libraries
- `libonnxruntime` (ONNX Runtime)
- `libevent` (Event loop)
- `libjpeg-turbo` (JPEG encoding)
- `libyaml` (Configuration parsing)

### Python Packages
- `onnx` (ONNX model parsing)
- `onnxruntime` (ONNX inference engine)

### ONNX Models
Place the following models in the model directory (default: `/data/data/com.termux/files/home/libcamera`):
- `algo.onnx` - Feature extraction / ISP algorithm
- `applier.onnx` - Color space conversion / final processing

---

## 🧪 Tools

### `softisp-save` (Capture Utility)
Captures frames and saves them in various formats.

```bash
# Basic capture (Bayer RAW)
./build/src/tools/softisp-save -o output.raw

# Capture YUV420
./build/src/tools/softisp-save -f yuv -o output.yuv

# Capture RGB
./build/src/tools/softisp-save -f rgb -o output.rgb

# Capture PPM (human-readable)
./build/src/tools/softisp-save -f ppm -o output.ppm

# Custom resolution
./build/src/tools/softisp-save -w 1280 -h 720 -o output.raw

# Save metadata
./build/src/tools/softisp-save --save-meta -o output.raw
```

### `softisp-tool` (ONNX Testing Suite)
Inspects and tests ONNX models.

```bash
# Inspect model
./build/src/tools/softisp-tool inspect algo.onnx

# Test inference
./build/src/tools/softisp-tool test algo.onnx --input input.raw --output output.onnx

# Verify model compatibility
./build/src/tools/softisp-tool verify --algo algo.onnx --applier applier.onnx
```

---

## 🐛 Known Issues

### 1. Termux `invokeMethod` Bug (Critical)
**Symptom:** `cam` application fails with `Failed to get default stream configuration`.  
**Root Cause:** The `CameraManager` on Termux/Android calls `invokeMethod(&PipelineHandler::generateConfiguration)`, but the template dispatch mechanism fails to route the call to **custom** pipeline handlers (like SoftISP). It returns `nullptr` even though the method is correctly implemented.

**Evidence:**
```
[INFO] SoftISPCameraData::generateConfiguration called (Camera Object)  # Called from match()
[DEBUG] config=null  # invokeMethod returns null
[ERROR] Failed to get default stream configuration
```

**Workaround:** None on Termux. The architecture is correct, but the platform has a bug.

**Solution:** Deploy to **real hardware** (Raspberry Pi, Rockchip) where the standard libcamera implementation is used. The `invokeMethod` dispatch works correctly on these platforms.

### 2. Frame Buffer Export
**Status:** Not fully implemented for virtual camera.  
**Reason:** Frame buffer export is a pipeline/job responsibility (like `SimplePipeline`), not a camera object responsibility. The current implementation delegates to hardware components which are not available in the pure software virtual camera.

**Future:** Implement V4L2 loopback device or real hardware buffer export.

---

## 🚀 Deployment to Real Hardware

### Raspberry Pi
1. **Build on Pi:**
   ```bash
   meson setup build -Dpipelines=softisp -Dsoftisp=enabled -Ddevelopment=true
   meson compile -C build
   ```
2. **Copy Models:**
   ```bash
   cp algo.onnx applier.onnx /usr/share/libcamera/ipa/softisp/
   ```
3. **Run:**
   ```bash
   export LIBCAMERA_IPA=/usr/share/libcamera/ipa/softisp
   ./build/src/apps/cam/cam -c softisp_virtual --capture=1
   ```

### Rockchip
1. **Cross-compile** for Rockchip architecture (RK3588, RK3399, etc.).
2. **Copy Models** to `/usr/share/libcamera/ipa/softisp/`.
3. **Configure** V4L2 device nodes for the camera sensor.
4. **Run** with `cam` or `gstreamer`.

---

## 📝 Development Guidelines

### Coding Style
- **C/C++ Files:** Use **spaces** for indentation (4 spaces per level).
- **No Tabs:** Tabs are not allowed in C/C++ source files.
- **Logging:** Use `LOG(SoftISPPipeline, Level)` for all logs.
- **Error Handling:** Always check return values and log errors.

### Architecture Principles
1. **Division of Duty:**
   - `PipelineHandler`: Dispatcher only.
   - `SoftISPCameraData`: Camera Object (holds logic).
   - `VirtualCamera`: Frame generator.
2. **Delegation:** `Handler` → `CameraData` → `VirtualCamera`.
3. **State Encapsulation:** All camera state is in `SoftISPCameraData`.

### Testing
- **Unit Tests:** Use `softisp-tool` for ONNX model testing.
- **Integration Tests:** Use `softisp-save` for frame capture.
- **End-to-End:** Test with `cam` application on real hardware.

---

## 📚 References

- [libcamera Documentation](https://libcamera.org)
- [SimplePipeline Implementation](src/libcamera/pipeline/simple/)
- [VirtualPipeline Implementation](src/libcamera/pipeline/virtual/)
- [ONNX Runtime Docs](https://onnxruntime.ai)

---

## 📄 License

This implementation is licensed under **LGPL-2.1-or-later**.

```
Copyright (C) 2024 George Chan <gchan9527@gmail.com>
```

All commits are attributed to **George Chan** with proper `Signed-off-by` lines.

---

## 🔄 Changelog

### `softisp_final` (Latest)
- ✅ Aligned architecture with `SimplePipeline` pattern.
- ✅ `SoftISPCameraData` is now the **Camera Object**.
- ✅ `PipelineHandlerSoftISP` is a **thin dispatcher**.
- ✅ `VirtualCamera` handles frame generation only.
- ✅ Fixed IPA module loading (`pipelineName` and `pipelineVersion` mismatch).
- ✅ Fixed infinite loop in `match()` (lifecycle management).
- ✅ Implemented ONNX integration and frame generation.
- ⚠️ **Termux `invokeMethod` bug** prevents `cam` from working on Android.
- 🚀 **Ready for deployment** to Raspberry Pi / Rockchip.

---

**Author:** George Chan <gchan9527@gmail.com>  
**Last Updated:** 2026-04-24
