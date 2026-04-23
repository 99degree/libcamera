# SoftISP IPA Implementation Plan

## Objective
Implement the actual ONNX inference logic in `src/ipa/softisp/softisp.cpp` to process camera frames using the dual-model architecture.

## Architecture Overview
```
Camera Frame → algo.onnx (coefficient generation) → Coefficients → applier.onnx (image processing) → Processed Frame
```

## Current Status
- ✅ Pipeline handler implemented (`src/libcamera/pipeline/softisp/softisp.cpp`)
- ✅ VirtualCamera component ready
- ✅ Mojom interface generated
- ✅ ONNX Runtime integration stubs in place
- ✅ Build system configured
- ❌ Actual ONNX inference logic not implemented

## Implementation Steps

### Phase 1: Model Loading & Initialization
1. **Load ONNX Models**
   - Load `algo.onnx` and `applier.onnx` from `SOFTISP_MODEL_DIR`
   - Validate model inputs/outputs match expected shapes
   - Store model sessions and metadata

2. **Initialize Inference Environment**
   - Create ONNX Runtime environment
   - Configure session options (CPU/GPU, optimization level)
   - Allocate input/output tensors

### Phase 2: processStats() Implementation
1. **Extract Frame Metadata**
   - Get frame dimensions, format, sensor data
   - Prepare input tensors for `algo.onnx`

2. **Run algo.onnx Inference**
   - Prepare input tensors:
     - `image_desc.input.image.function`
     - `image_desc.input.width.function`
     - `image_desc.input.frame_id.function`
     - `blacklevel.offset.function`
   - Execute inference
   - Extract coefficient outputs:
     - `awb.wb_gains.function` (White Balance)
     - `ccm.ccm.function` (Color Correction Matrix)
     - `tonemap.tonemap_curve.function`
     - `gamma.gamma_value.function`
     - etc.

3. **Return Statistics**
   - Populate `ControlList` with computed coefficients
   - Return via `processStats()`

### Phase 3: processFrame() Implementation
1. **Prepare Input Tensors**
   - Map buffer memory
   - Convert image data to ONNX input format
   - Apply coefficients from `processStats()`

2. **Run applier.onnx Inference**
   - Prepare inputs:
     - Raw image buffer
     - Coefficients from stats
   - Execute inference
   - Extract output tensor

3. **Write Output**
   - Copy processed data back to buffer
   - Unmap memory

### Phase 4: Optimization & Testing
1. **Performance Optimization**
   - Implement tensor caching
   - Optimize memory allocations
   - Consider async inference

2. **Testing**
   - Test with VirtualCamera patterns
   - Validate output quality
   - Benchmark performance

## File Structure
```
src/ipa/softisp/
├── softisp.cpp        # Main IPA implementation (TO BE COMPLETED)
├── softisp.h          # Header file
├── onnx_engine.cpp    # NEW: ONNX runtime wrapper (optional)
├── onnx_engine.h      # NEW: ONNX runtime wrapper header
└── meson.build        # Build configuration
```

## Key Challenges
1. **Tensor Shape Handling** - Dynamic shapes for image inputs
2. **Memory Management** - Efficient buffer mapping/unmapping
3. **Data Conversion** - Raw Bayer to ONNX tensor format
4. **Performance** - Real-time processing requirements

## Dependencies
- ONNX Runtime library
- libcamera IPA framework
- OpenCV (optional, for image processing helpers)

## Success Criteria
- ✅ `processStats()` returns valid coefficients
- ✅ `processFrame()` processes frames correctly
- ✅ Performance meets real-time requirements (>30fps)
- ✅ No memory leaks
- ✅ Handles various image resolutions

## Next Immediate Tasks
1. Implement `OnnxEngine` class for model loading/inference
2. Complete `processStats()` with algo.onnx inference
3. Implement `processFrame()` with applier.onnx inference
4. Test with VirtualCamera
