# SoftISP Implementation Guide

## Overview

This guide documents the SoftISP Image Processing Algorithm (IPA) implementation for libcamera, featuring dual ONNX model support for ISP coefficient generation and application.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    Test Application                          │
│  - Creates camera using pipeline handler                     │
│  - Allocates buffers via FrameBufferAllocator                │
│  - Queues requests and waits for completion                  │
│  - Processes frames through IPA                              │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│              Pipeline Handler (dummysoftisp)                 │
│  - match() - Creates virtual camera (no hardware needed)     │
│  - configure() - Sets up streams, initializes IPA            │
│  - queueRequestDevice() - Routes to processRequest()         │
│  - processRequest() - Calls ipa_->processStats()             │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                    IPA Proxy (Threaded)                      │
│  - Loads ipa_dummysoftisp.so module                          │
│  - Creates SoftIsp algorithm object                          │
│  - Routes init/configure/start/processStats calls            │
│  - Manages processing thread                                 │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│                  SoftISP Algorithm (SoftIsp)                 │
│  - init() - Loads algo.onnx and applier.onnx                 │
│  - configure() - Sets up for processing                      │
│  - start() - Activates IPA processing                        │
│  - processStats() - [TODO] Run ONNX inference pipeline       │
└─────────────────────────────────────────────────────────────┘
```

## Key Components

### 1. IPA Module (`src/ipa/softisp/`)

**Files:**
- `softisp.h` - Algorithm class declaration
- `softisp.cpp` - Implementation with ONNX Runtime integration
- `meson.build` - Build configuration

**Features:**
- Loads `algo.onnx` (ISP coefficient generation)
- Loads `applier.onnx` (coefficient application)
- Uses ONNX Runtime (`Ort::Session`) for inference
- Threaded processing via IPA proxy

**Model Requirements:**
- `algo.onnx`: 4 inputs, 15 outputs
- `applier.onnx`: 10 inputs, 7 outputs
- Models must be in `SOFTISP_MODEL_DIR` directory

### 2. Pipeline Handlers

**Real Camera (`src/libcamera/pipeline/softisp/`):**
- Matches `/dev/video0` devices
- Follows standard libcamera pipeline pattern
- Uses actual V4L2 hardware

**Dummy Camera (`src/libcamera/pipeline/dummysoftisp/`):**
- Creates virtual camera without hardware
- Programmatically registers camera with CameraManager
- Uses `memfd_create` for buffer allocation
- Perfect for testing and development

### 3. Test Application (`tools/softisp-test-app.cpp`)

**Usage:**
```bash
./build/tools/softisp-test-app --pipeline dummysoftisp --frames 10
```

**Options:**
- `-p, --pipeline <name>`: Pipeline handler (softisp or dummysoftisp)
- `-f, --frames <n>`: Number of frames to capture
- `-o, --output <dir>`: Output directory for saved frames
- `-P, --pattern <type>`: Test pattern (gradient, checker, solid)
- `-l, --list`: List available cameras
- `-v, --verbose`: Verbose output

## Build Instructions

### Prerequisites
- libcamera source tree
- ONNX Runtime development libraries
- Meson build system

### Configuration
```bash
meson setup build \
  -Dsoftisp=enabled \
  -Dpipelines='softisp,dummysoftisp' \
  -Dtest=true \
  -Dc_args=-Wno-error \
  -Dcpp_args='-Wno-error -DDEVELOPMENT_MODE'
```

### Build
```bash
meson compile -C build
```

### Environment Setup
```bash
export LD_LIBRARY_PATH=/path/to/libcamera/build/src/libcamera:/path/to/libcamera/build/src/ipa/dummysoftisp:$LD_LIBRARY_PATH
export SOFTISP_MODEL_DIR=/path/to/onnx/models
```

## Verification

Run the verification script:
```bash
./verify_softisp.sh
```

Expected output:
```
✓ IPA modules built successfully
✓ Symbol exports correct
✓ Pipeline handlers registered
✓ Test application built
✓ ONNX models found
```

## Test Run

```bash
./build/tools/softisp-test-app --pipeline dummysoftisp --frames 5
```

Expected output:
```
Camera started
Processing 5 frames...
[DEBUG] processRequest() START
[DEBUG] Calling ipa_->processStats() for frame 0
[DEBUG] ipa_->processStats() completed
SoftIsp: Frame 0 processed (inference logic to be implemented)
Frame 1/5 - Request queued and completed
...
Capture complete. Total frames: 5
```

## Development Notes

### Termux Compatibility

The implementation includes several Termux-specific patches:

1. **IPA Loading**: Modified `IPAManager::isSignatureValid()` to return `true` when `HAVE_IPA_PUBKEY` is not defined, allowing direct in-process loading without `fork()`.

2. **Buffer Allocation**: Uses `memfd_create()` with `mkstemp()` fallback for shared memory buffers.

3. **Missing Symbols**: Handles missing `pthread_cancel` and `strverscmp` in Termux environment.

### Buffer Handling

The test app uses a simplified sleep-based completion approach:
```cpp
camera->queueRequest(request.get());
std::this_thread::sleep_for(std::chrono::milliseconds(100));
```

For production use, implement event-driven completion using the `requestCompleted` signal:
```cpp
camera->requestCompleted.connect([](Request *request) {
    // Process completed frame
});
```

### Future Improvements

1. **Actual ONNX Inference**: Implement tensor preparation and execution in `SoftIsp::processStats()`
2. **Event-Driven Test App**: Replace sleep-based completion with signal handling
3. **DMABuf Support**: Replace shared memory with proper DMABuf allocation
4. **Real Statistics**: Extract actual sensor statistics instead of synthetic data

## Troubleshooting

### IPA Module Not Loading
- Check `SOFTISP_MODEL_DIR` environment variable
- Verify `ipa_dummysoftisp.so` is in library path
- Check logs for "SoftISP IPA module loaded" message

### Models Not Found
- Ensure `algo.onnx` and `applier.onnx` exist in `SOFTISP_MODEL_DIR`
- Check file permissions
- Verify ONNX Runtime is linked correctly

### Buffer Allocation Failed
- Check available memory
- Verify `memfd_create` support (fallback to `mkstemp`)
- Check `/dev/shm` availability

## References

- [libcamera IPA Documentation](https://libcamera.org/)
- [ONNX Runtime API](https://onnxruntime.ai/docs/api/c/)
- [Virtual Camera Pipeline](src/libcamera/pipeline/virtual/)
- [RKISP1 Pipeline](src/libcamera/pipeline/rkisp1/)

## Status

- ✅ IPA module loading (Termux compatible)
- ✅ ONNX model loading
- ✅ Full IPA lifecycle (init → configure → start)
- ✅ Frame processing pipeline
- ✅ End-to-end frame completion
- ⏳ Actual ONNX inference (placeholder ready)

## License

GPL-2.0-or-later
