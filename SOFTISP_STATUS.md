# SoftISP Implementation Status

## Overview
SoftISP is a complete ONNX-based Image Processing Algorithm (IPA) for libcamera that implements a dual-model pipeline for real-time image processing.

## Architecture

### Components
1. **IPA Module** (`src/ipa/softisp/`)
   - `softisp.cpp`: Main algorithm implementation with ONNX inference
   - `softisp_module.cpp`: IPA module entry point
   - `softisp.h`: Algorithm interface and public API
   - `softisp_wrapper.h`: Auto-generated wrapper (from mojom)

2. **Pipeline Handlers**
   - `src/libcamera/pipeline/softisp/`: Real camera pipeline
   - `src/libcamera/pipeline/dummysoftisp/`: Dummy/virtual camera pipeline

3. **Tools**
   - `tools/softisp-save.cpp`: Frame capture and saving utility
   - `tools/softisp-onnx-test.cpp`: ONNX model inspector
   - `tools/softisp-onnx-inference-test.cpp`: Full inference verification
   - `softisp_capture.sh`: Helper script for frame capture

## Features

### ✅ Implemented
- **Dual-model ONNX inference pipeline**
  - `algo.onnx`: ISP coefficient generation (4 inputs, 15 outputs)
  - `applier.onnx`: Coefficient application (10 inputs, 7 outputs)
  - Efficient tensor passing via `Ort::IoBinding`

- **Complete pipeline integration**
  - Camera creation and registration
  - Configuration generation and validation
  - Buffer allocation and management
  - Request processing and completion

- **Test infrastructure**
  - Model loading and validation
  - Full inference verification
  - Frame capture with RGB/YUV output
  - Metadata saving (JSON)

- **Build system integration**
  - Meson feature flag (`-Dsoftisp=enabled`)
  - Optional compilation (default disabled)
  - Automatic wrapper generation from `.mojom` files

### ⚠️ In Progress / Limitations

#### Buffer Writing
The ONNX inference runs successfully and produces output tensors, but the buffer writing is limited:
- Current: Only 1 output element is stored/retrieved
- Issue: The `applier.onnx` output tensor shape needs investigation
- Solution: Complete the `getProcessedOutput()` implementation and fix tensor shape handling

#### IPC Between Pipeline and IPA
The pipeline and IPA are separate shared objects, which limits direct memory access:
- Workaround: Use `getProcessedOutput()` to retrieve data from IPA
- Future: Consider shared memory or callback mechanism for large buffers

## Build Instructions

```bash
# Setup build directory
meson setup build -Dsoftisp=enabled -Dpipelines='softisp,dummysoftisp' -Dtest=true -Dc_args=-Wno-error -Dcpp_args='-Wno-error'

# Build
ninja -C build

# Required environment variables
export LD_LIBRARY_PATH=build/src/libcamera:build/src/ipa/dummysoftisp:$LD_LIBRARY_PATH
export SOFTISP_MODEL_DIR=/path/to/softisp_models
```

## Usage

### List Cameras
```bash
libcamera-cam --list
# Should show "SoftISP Dummy Camera" (ID 1)
```

### Capture Frames
```bash
# Using the helper script
./softisp_capture.sh SoftISP 3 ./output_frames

# Using the tool directly
./build/tools/softisp-save -c SoftISP -f 3 -o ./frames -n test
```

### Command-line Options
```
-c, --camera ID     Camera ID substring (default: SoftISP)
-f, --frames N      Number of frames (default: 3)
-o, --output DIR    Output directory (default: ./frames)
-r, --rgb           Save RGB output (default: enabled)
-y, --yuv           Save YUV output (default: enabled)
-m, --metadata      Save metadata JSON (default: enabled)
-n, --name NAME     Output filename prefix (default: frame)
-h, --help          Show help
```

### Output Files
For each captured frame:
- `frame_XXXX_rgb.bin`: RGB888 raw data (1920x1080x3 = 6,220,800 bytes)
- `frame_XXXX_yuv.bin`: YUV420 data (1920x1080x1.5 = 3,110,400 bytes)
- `frame_XXXX_meta.json`: Metadata with frame info and coefficients

## Verification

### ONNX Model Loading
```bash
./build/tools/softisp-onnx-test -m /path/to/algo.onnx
./build/tools/softisp-onnx-test -m /path/to/applier.onnx
```

### Full Inference Test
```bash
./build/tools/softisp-onnx-inference-test
# Expected: "Full Inference Pipeline SUCCEEDED"
```

### Pipeline Test
```bash
timeout 10 ./build/tools/softisp-save -c SoftISP -f 1 -o ./test
# Expected: "Frame 0 processed successfully"
```

## Model Specifications

### algo.onnx
- **Inputs**:
  1. Image data (int16, shape: [-1, -1])
  2. Width (int64, shape: [1])
  3. Frame ID (int64, shape: [1])
  4. Black level (float32, shape: [1])
- **Outputs**: 15 tensors (ISP coefficients)
  - Output 2: AWB gains (3 floats)
  - Output 4: CCM matrix (9 floats)
  - Output 5: Tonemap curve (16 floats)
  - Output 6: Gamma value (1 float)
  - Output 12: RGB2YUV matrix (9 floats)
  - Output 14: Chroma subsample scale (1 float)

### applier.onnx
- **Inputs**: 10 tensors
  - 4 original inputs from algo.onnx
  - 6 coefficient tensors from algo.onnx outputs
- **Outputs**: 7 tensors
  - Output 0: Processed image data (target: 1920x1080x3 RGB)

## Next Steps

1. **Complete buffer writing**
   - Fix tensor shape handling in `getProcessedOutput()`
   - Implement full buffer write in pipeline
   - Test with actual image data

2. **Replace synthetic data**
   - Use real frame buffer data as input to algo.onnx
   - Use real sensor statistics instead of placeholder data

3. **Performance optimization**
   - Pre-allocate ONNX tensors
   - Implement tensor pooling
   - Optimize memory transfers

4. **Event-driven completion**
   - Replace sleep-based waits with `requestCompleted` signals
   - Improve test app responsiveness

## Files Modified

### Core Implementation
- `src/ipa/softisp/softisp.cpp` (500+ lines)
- `src/ipa/softisp/softisp.h`
- `src/ipa/softisp/softisp_module.cpp`
- `src/ipa/softisp/meson.build`

### Pipeline Handlers
- `src/libcamera/pipeline/softisp/softisp.cpp`
- `src/libcamera/pipeline/dummysoftisp/softisp.cpp`
- `src/libcamera/pipeline/dummysoftisp/softisp.h`

### Tools
- `tools/softisp-save.cpp`
- `tools/softisp-onnx-test.cpp`
- `tools/softisp-onnx-inference-test.cpp`

### Build System
- `meson_options.txt`
- `src/meson.build`
- `src/ipa/meson.build`
- `src/libcamera/pipeline/meson.build`

### Documentation
- `SOFTISP_IMPLEMENTATION_STATUS.md`
- `SOFTISP_BUFFER_FIX.md`
- `WRAPPER_GENERATOR_SKILLS.md`
- `SOFTISP_STATUS.md` (this file)

## License
GPL-2.0-or-later (consistent with libcamera)
