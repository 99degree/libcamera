# SoftISP IPA ONNX Implementation Plan

## Current State
✅ Pipeline handler fully functional  
✅ Virtual camera works without hardware  
✅ IPA stub module built and integrated  
✅ IPA framework in place (loadIPA, processStats, processFrame)  
⚠️ ONNX runtime not integrated  
⚠️ ONNX models not loaded or executed  

## Goals
Implement full ONNX-based image processing in the SoftISP IPA module:
1. Integrate ONNX Runtime C++ API
2. Load and initialize `algo.onnx` and `applier.onnx` models
3. Implement `processStats()` to run algo model
4. Implement `processFrame()` to run applier model
5. Handle model inputs/outputs correctly
6. Add error handling and logging

## Implementation Steps

### Step 1: Add ONNX Runtime Dependency
- [ ] Install ONNX Runtime development libraries
- [ ] Update `meson.build` to find and link ONNX Runtime
- [ ] Add ONNX Runtime headers to include path
- [ ] Handle optional ONNX Runtime (graceful fallback if not available)

### Step 2: Update IPA Module Header
- [ ] Replace stub header with full interface
- [ ] Add ONNX session member variables
- [ ] Add model path configuration
- [ ] Add input/output tensor definitions

### Step 3: Implement ONNX Engine Wrapper
- [ ] Create `OnnxEngine` class to manage ONNX sessions
- [ ] Implement model loading with error handling
- [ ] Implement tensor allocation and memory management
- [ ] Add input/output name extraction from models
- [ ] Implement inference execution

### Step 4: Implement `init()` Method
- [ ] Load `algo.onnx` model
- [ ] Load `applier.onnx` model
- [ ] Extract input/output names from both models
- [ ] Initialize ONNX runtime environment
- [ ] Validate model compatibility with sensor info
- [ ] Set up memory allocators

### Step 5: Implement `processStats()` Method
- [ ] Prepare input tensors from raw frame/stats
- [ ] Run `algo.onnx` inference
- [ ] Extract output tensors (ISP parameters)
- [ ] Parse and validate outputs
- [ ] Populate `ControlList` with statistics
- [ ] Handle errors gracefully

### Step 6: Implement `processFrame()` Method
- [ ] Prepare input tensors (frame buffer + ISP parameters)
- [ ] Run `applier.onnx` inference
- [ ] Write output to frame buffer
- [ ] Handle different pixel formats
- [ ] Validate output dimensions
- [ ] Handle errors gracefully

### Step 7: Add Configuration Support
- [ ] Support model path configuration via environment variable
- [ ] Add model validation on load
- [ ] Support model hot-reloading (optional)
- [ ] Add configuration file support for model parameters

### Step 8: Testing and Validation
- [ ] Test with sample ONNX models
- [ ] Validate output quality
- [ ] Performance benchmarking
- [ ] Error case testing
- [ ] Integration testing with pipeline

## Technical Details

### ONNX Models Expected
1. **algo.onnx** - Algorithm model
   - Input: Raw statistics, sensor parameters
   - Output: ISP coefficients (AWB, CCM, Gamma, Tone Map, etc.)

2. **applier.onnx** - Applier model
   - Input: Frame buffer, ISP coefficients
   - Output: Processed frame

### Memory Management
- Use ONNX Runtime memory allocators
- Avoid unnecessary memory copies
- Reuse buffers where possible
- Handle different buffer formats (NV12, RGB, etc.)

### Error Handling
- Model load failures
- Invalid input dimensions
- Inference failures
- Memory allocation failures
- Graceful degradation to stub mode

## Dependencies
- ONNX Runtime C++ API (v1.14+)
- YAML parser for configuration (optional)
- OpenCV (optional, for testing/visualization)

## Timeline
- Week 1: ONNX Runtime integration and model loading
- Week 2: Implement processStats() with algo.onnx
- Week 3: Implement processFrame() with applier.onnx
- Week 4: Testing, optimization, and documentation

## Success Criteria
- [ ] IPA module loads ONNX models successfully
- [ ] processStats() returns valid statistics
- [ ] processFrame() produces processed output
- [ ] No memory leaks or crashes
- [ ] Performance acceptable for target resolution
- [ ] Graceful fallback if ONNX not available

## Notes
- Start with simple test models to validate the pipeline
- Use ONNX Runtime's debug mode for troubleshooting
- Profile memory usage and inference time
- Document model input/output formats clearly
- Consider GPU acceleration if ONNX Runtime supports it
