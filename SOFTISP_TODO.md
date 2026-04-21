# SoftISP Implementation TODO List

This document tracks all remaining work needed to complete the SoftISP IPA module.

---

## 🎯 **Priority 1: ONNX Runtime Integration**

### 1.1 Add ONNX Runtime Dependency
- [ ] Add ONNX Runtime to `meson.build` in `src/ipa/softisp/`
- [ ] Check for ONNX Runtime installation (`onnxruntime`)
- [ ] Link against `onnxruntime` library
- [ ] Handle missing ONNX Runtime gracefully (build without it)

### 1.2 Load ONNX Models
- [ ] Implement model loading in `SoftIsp::init()` or `SoftIsp::start()`
- [ ] Read `algo.onnx` from `SOFTISP_MODEL_DIR` environment variable
- [ ] Read `applier.onnx` from same directory
- [ ] Validate model files exist and are valid
- [ ] Cache model paths for reuse

### 1.3 Initialize ONNX Session
- [ ] Create ONNX Runtime environment
- [ ] Create inference session for `algo.onnx`
- [ ] Create inference session for `applier.onnx`
- [ ] Set up input/output tensors
- [ ] Handle initialization errors

### 1.4 Implement `processStats()` - algo.onnx
- [ ] Extract statistics from input frame
- [ ] Prepare input tensors (4 inputs)
- [ ] Run inference on `algo.onnx`
- [ ] Extract 15 outputs (ISP coefficients)
- [ ] Store coefficients for use in next step

### 1.5 Implement `processStats()` - applier.onnx
- [ ] Prepare input tensors (10 inputs including coefficients)
- [ ] Run inference on `applier.onnx`
- [ ] Extract 7 outputs (processed metadata)
- [ ] Apply metadata to request

### 1.6 Error Handling
- [ ] Handle model loading failures
- [ ] Handle inference errors
- [ ] Log detailed error messages
- [ ] Fallback to default coefficients on error

---

## 🎯 **Priority 2: Buffer Allocation**

### 2.1 Implement `exportFrameBuffers()` (Real Pipeline)
- [ ] Use `DMABufAllocator` to allocate buffers
- [ ] Create `FrameBuffer` objects with allocated buffers
- [ ] Set buffer metadata (size, stride, etc.)
- [ ] Handle allocation failures
- [ ] Clean up on error

### 2.2 Implement `exportFrameBuffers()` (Dummy Pipeline)
- [ ] Allocate memory buffers (can use shared memory)
- [ ] Create `FrameBuffer` objects
- [ ] Set buffer metadata
- [ ] Store buffer references for later use

### 2.3 Buffer Cleanup
- [ ] Implement buffer cleanup in `CameraData` destructor
- [ ] Release DMA buffers properly
- [ ] Handle partial cleanup on error

---

## 🎯 **Priority 3: V4L2 Integration (Real Pipeline)**

### 3.1 Device Enumeration
- [ ] Enumerate V4L2 devices in `match()`
- [ ] Filter for cameras that support SoftISP
- [ ] Create `Camera` objects for each device

### 3.2 Device Configuration
- [ ] Open V4L2 video device in `configure()`
- [ ] Query and set supported formats
- [ ] Configure resolution and pixel format
- [ ] Set buffer queue depth

### 3.3 Streaming Control
- [ ] Implement `start()` to begin V4L2 streaming
- [ ] Queue initial buffers
- [ ] Implement `stopDevice()` to stop streaming
- [ ] Flush buffer queue on stop

### 3.4 Request Handling
- [ ] Implement `queueRequestDevice()` properly
- [ ] Queue buffer to V4L2 device
- [ ] Wait for buffer completion
- [ ] Extract statistics from completed buffer
- [ ] Pass to IPA module
- [ ] Complete request

---

## 🎯 **Priority 4: Test Application**

### 4.1 Fix API Usage
- [ ] Update `softisp-test-app.cpp` to use correct Camera API
- [ ] Replace deprecated methods
- [ ] Fix `pipelineHandler()` usage
- [ ] Fix `exportFrameBuffers()` usage

### 4.2 Implement Frame Capture
- [ ] Allocate frame buffers
- [ ] Start camera streaming
- [ ] Capture frames in loop
- [ ] Save frames to disk (YUV/RAW)
- [ ] Add frame counter and timing

### 4.3 Add Command-Line Options
- [ ] `--pipeline` - Select pipeline (softisp/dummysoftisp)
- [ ] `--output` - Output file path
- [ ] `--frames` - Number of frames to capture
- [ ] `--resolution` - Camera resolution
- [ ] `--model-dir` - Path to ONNX models

### 4.4 Add Verification
- [ ] Verify output file is created
- [ ] Check frame format and size
- [ ] Optional: Display statistics

---

## 🎯 **Priority 5: Unit Tests**

### 5.1 IPA Module Tests
- [ ] Complete `test/ipa/softisp_module_test.cpp`
- [ ] Test `ipaCreate()` returns valid module
- [ ] Test `init()`, `start()`, `stop()` lifecycle
- [ ] Test `processStats()` with mock data
- [ ] Test model loading and inference

### 5.2 Virtual Pipeline Tests
- [ ] Complete `test/ipa/softisp_virtual_module_test.cpp`
- [ ] Test virtual module registration
- [ ] Test with dummy camera data

### 5.3 Integration Tests
- [ ] Test full pipeline with dummy camera
- [ ] Test buffer allocation and release
- [ ] Test request queue and completion
- [ ] Test error handling

---

## 🎯 **Priority 6: Documentation**

### 6.1 User Documentation
- [ ] Write README for SoftISP module
- [ ] Document build requirements (ONNX Runtime)
- [ ] Document environment variables (`SOFTISP_MODEL_DIR`)
- [ ] Document usage examples

### 6.2 Developer Documentation
- [ ] Document IPA module architecture
- [ ] Document ONNX model format requirements
- [ ] Document extension points
- [ ] Add code comments for complex logic

### 6.3 Model Documentation
- [ ] Document `algo.onnx` input/output specifications
- [ ] Document `applier.onnx` input/output specifications
- [ ] Document coefficient format and usage
- [ ] Provide example model files

---

## 🎯 **Priority 7: Performance Optimization**

### 7.1 Inference Optimization
- [ ] Use ONNX Runtime execution providers (CUDA, TensorRT)
- [ ] Optimize tensor memory layout
- [ ] Batch processing if supported
- [ ] Pre-allocate output tensors

### 7.2 Pipeline Optimization
- [ ] Reduce memory copies
- [ ] Use zero-copy buffers where possible
- [ ] Optimize thread synchronization
- [ ] Profile and optimize hot paths

### 7.3 Resource Management
- [ ] Pool ONNX inference sessions
- [ ] Cache model data
- [ ] Minimize allocation in hot paths
- [ ] Implement lazy initialization

---

## 🎯 **Priority 8: Testing & Validation**

### 8.1 Functional Testing
- [ ] Test with real camera hardware
- [ ] Test with various resolutions
- [ ] Test with different lighting conditions
- [ ] Compare output with reference ISP

### 8.2 Performance Testing
- [ ] Measure frame rate
- [ ] Measure latency
- [ ] Measure CPU/GPU usage
- [ ] Compare with hardware ISP

### 8.3 Quality Testing
- [ ] Evaluate image quality
- [ ] Test color accuracy
- [ ] Test noise reduction
- [ ] Test exposure handling

---

## 📋 **Quick Wins (Easy Fixes)**

These are quick fixes that don't require major implementation:

- [ ] Add `[[maybe_unused]]` attributes to suppress warnings
- [ ] Add better error messages for model loading failures
- [ ] Add debug logging for IPA processing
- [ ] Add configuration file support
- [ ] Add support for multiple camera models

---

## 🚧 **Blocked Items**

- [ ] ONNX Runtime not installed on build system
  - **Blocker**: Cannot test ONNX integration
  - **Solution**: Install ONNX Runtime or use mock models

- [ ] No real camera hardware for testing
  - **Blocker**: Cannot test V4L2 integration
  - **Solution**: Use dummy pipeline or obtain camera

- [ ] No reference ISP output for comparison
  - **Blocker**: Cannot validate image quality
  - **Solution**: Use standard test patterns

---

## 📊 **Progress Tracking**

| Category | Total Items | Completed | In Progress | Blocked |
|----------|-------------|-----------|-------------|---------|
| ONNX Integration | 6 | 0 | 0 | 1 |
| Buffer Allocation | 3 | 0 | 0 | 0 |
| V4L2 Integration | 4 | 0 | 0 | 1 |
| Test App | 4 | 0 | 0 | 0 |
| Unit Tests | 3 | 0 | 0 | 0 |
| Documentation | 3 | 1 | 0 | 0 |
| Performance | 3 | 0 | 0 | 0 |
| Testing | 3 | 0 | 0 | 1 |
| Quick Wins | 5 | 0 | 0 | 0 |
| **Total** | **34** | **1** | **0** | **3** |

---

*Last Updated: 2026-04-21*
*Current Phase: Build Complete, Ready for ONNX Integration*
