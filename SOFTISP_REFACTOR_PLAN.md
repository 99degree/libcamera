# SoftISP Implementation Status and Refactoring Plan

## Current State Analysis

### AF Algorithm Integration
- ✅ Hardware-agnostic AF algorithm implemented (`AfAlgo` class)
- ✅ CPU-based AF statistics calculator (`AfStatsCalculator`)
- ✅ Simple IPA pipeline integration (`Af` algorithm)
- ⚠️  Frame data access not yet connected (placeholder contrast values used)
- ⚠️  PDAF integration pending
- ⚠️  VCM driver interface pending

### SoftISP Pipeline Implementation
- ✅ Virtual camera implementation with frame generation
- ✅ ONNX-based IPA module with dual model support
- ✅ Pipeline handler with both real and virtual camera support
- ✅ IPA interface integration with virtual camera
- ⚠️  Real camera V4L2 integration needs refinement
- ⚠️  Frame data access to AfStatsCalculator pending

### Key Components

#### 1. libipa::AfAlgo
Location: `src/ipa/libipa/af_algo.*`
- Implements hill climbing and PDAF algorithms
- Supports Manual/Auto/Continuous modes
- Configurable via INI files
- ControlList integration

#### 2. libipa::AfStatsCalculator
Location: `src/ipa/libipa/af_stats.*`
- CPU-based contrast calculation from raw frames
- Multiple methods: Sobel, Laplacian, Variance
- ROI support for weighted focus areas

#### 3. ipa::soft::algorithms::Af
Location: `src/ipa/simple/algorithms/af.*`
- Simple pipeline integration of AfAlgo
- ControlList handling for lens position controls
- Metadata reporting (AfState, LensPosition)

#### 4. VirtualCamera
Location: `src/libcamera/pipeline/softisp/virtual_camera.cpp`
- Generates synthetic Bayer frames
- Implements frame queue and processing
- IPA interface integration

#### 5. SoftIsp (ONNX-based)
Location: `src/ipa/softisp/softisp.*`
- Dual ONNX model architecture (algo/applier)
- Stats processing and frame processing
- ONNX Runtime integration

#### 6. PipelineHandlerSoftISP
Location: `src/libcamera/pipeline/softisp/softisp.*`
- Supports both real and virtual cameras
- Placeholder stream abstraction
- IPA loading and configuration

## Refactoring Goals

### 1. Abstract Camera Interface
Make Camera and PipelineHandler classes more abstract to decouple:
- CameraData owns IPA, Pipeline accesses via getter
- VirtualCamera manages its own lifecycle independently
- Clear separation between real and virtual implementations

### 2. Frame Data Access
Connect AfStatsCalculator to actual frame data:
- Modify process() to receive raw frame pointer
- Or calculate stats in separate pass before process()

### 3. PDAF Integration
Add phase detection support:
- Define PDAF statistics structure
- Calculate phase error from PDAF pixels
- Pass to AfAlgo::process(contrast, phaseError, confidence)

### 4. VCM Driver Interface
Map lens position to hardware:
- Connect controls::LensPosition to VCM driver
- Handle VCM calibration data

## Implementation Plan

### Phase 1: Architecture Refinement
1. Make CameraData fully independent with Thread inheritance
2. Decouple VirtualCamera lifecycle from pipeline handler
3. Implement proper buffer ownership and lifecycle management
4. Add proper error handling and state management

### Phase 2: AF Integration Completion
1. Connect AfStatsCalculator to actual frame data
2. Implement PDAF statistics collection
3. Add VCM driver interface
4. Complete ControlList integration

### Phase 3: Testing and Validation
1. Add unit tests for AfAlgo
2. Add integration tests with simulated frames
3. Real hardware testing with VCM lens
4. Performance optimization

## Files to Refactor

### Core Architecture
- `src/libcamera/pipeline/softisp/softisp.h` - Main pipeline handler
- `src/libcamera/pipeline/softisp/softisp.cpp` - Main implementation
- `src/libcamera/pipeline/softisp/virtual_camera.cpp` - Virtual camera implementation

### AF Integration
- `src/ipa/libipa/af_stats.*` - Statistics calculator
- `src/ipa/simple/algorithms/af.*` - Simple pipeline integration
- `src/ipa/libipa/af_algo.*` - Core algorithm

### IPA Module
- `src/ipa/softisp/softisp.*` - ONNX-based implementation
- `src/ipa/softisp/onnx_engine.*` - ONNX Runtime wrapper

## Benefits of Refactored Design

1. **Modularity**: Clear separation of concerns between components
2. **Extensibility**: Easy to add new camera types or algorithms
3. **Maintainability**: Each component has a single responsibility
4. **Testability**: Components can be tested independently
5. **Performance**: Better buffer and thread management