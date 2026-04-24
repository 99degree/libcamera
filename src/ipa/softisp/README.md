# SoftISP IPA Module

## Architecture: Caller/Callee Pattern

### Design Principle
The SoftISP system follows a strict **Caller/Callee** pattern:

**CALLER (Pipeline Handler)**:
- Owns memory management
- Pre-calculates all buffer sizes
- Allocates FrameBuffers
- Maps buffers via `mmap()`
- Passes file descriptors (SharedFD) to IPA
- Handles cleanup (munmap, deallocation)

**CALLEE (IPA Module)**:
- **Stateless**: No internal state between calls
- **No allocation**: Uses pre-mapped buffers only
- **Read/Write**: Reads input, writes output to provided buffers
- **Processing**: Performs ONNX inference
- **Returns**: Results via metadata signal (for stats) or buffer write (for frames)

### Benefits
- **Performance**: No allocation during processing
- **Thread Safety**: Stateless IPA is inherently thread-safe
- **Predictability**: Fixed buffer sizes, no dynamic memory
- **Debugging**: Clear boundary between resource management and processing
- **Testing**: Easy to mock buffers for testing

## Two-Stage Processing

### Stage 1: `processStats()` (Callee)
**Caller Preparation:**
```cpp
// 1. Calculate stats buffer size
uint32_t statsSize = (width/4) * (height/4) * sizeof(uint32_t);

// 2. Allocate and map stats buffer
void *statsData = mmap(nullptr, statsSize, PROT_READ | PROT_WRITE, ...);

// 3. Populate with sensor statistics
populateStats(statsData, sensorData);
```

**Callee Processing:**
```cpp
void SoftIsp::processStats(...) {
    // Read from pre-mapped statsData (no allocation)
    // Run algo.onnx
    // Emit metadata via signal
    metadataReady.emit(frame, metadata);
}
```

### Stage 2: `processFrame()` (Callee)
**Caller Preparation:**
```cpp
// 1. Calculate frame buffer size
uint32_t frameSize = ((width * 10 + 7) / 8) * height; // SBGGR10

// 2. Allocate and map frame buffer
void *frameData = mmap(nullptr, frameSize, PROT_READ | PROT_WRITE, ...);

// 3. Populate with Bayer sensor data
populateBayer(frameData, sensorData);
```

**Callee Processing:**
```cpp
void SoftIsp::processFrame(...) {
    // Read Bayer from pre-mapped frameData (no allocation)
    // Apply AWB/AE parameters from Stage 1
    // Run applier.onnx
    // Write RGB/YUV back to frameData
}
```

## Data Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    CALLER (Pipeline)                        │
│  1. Calculate sizes (width, height, stride)                 │
│  2. Allocate FrameBuffers                                   │
│  3. Map buffers (mmap)                                      │
│  4. Populate with sensor data                               │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                    CALLEE (IPA) - Stateless                 │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │processStats  │ →  │ algo.onnx    │ →  │ metadata     │  │
│  │(read stats)  │    │ (AWB/AE)     │    │ (emit)       │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│                                                             │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │processFrame  │ →  │applier.onnx  │ →  │ write back   │  │
│  │(read Bayer)  │    │(ISP pipeline)│    │(RGB/YUV)     │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
└─────────────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                    CALLER (Pipeline)                        │
│  1. Unmap buffers (munmap)                                  │
│  2. Complete request                                        │
└─────────────────────────────────────────────────────────────┘
```

## Implementation Guidelines

### For Pipeline (Caller)
```cpp
// In processRequest():
for (const auto &buffer : buffers) {
    // 1. Calculate sizes
    uint32_t frameSize = calculateFrameSize(cfg);
    
    // 2. Map buffers
    void *data = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE, ...);
    
    // 3. Populate data
    populateBuffer(data, sensorData);
    
    // 4. Call IPA (stateless)
    ipa_->processStats(frameId, ...);
    ipa_->processFrame(frameId, fd, width, height, ...);
    
    // 5. Cleanup
    munmap(data, frameSize);
}
```

### For IPA (Callee)
```cpp
void SoftIsp::processStats(...) {
    // NO allocation
    // Read from provided buffer (already mapped by caller)
    // Run ONNX
    // Emit metadata
}

void SoftIsp::processFrame(...) {
    // NO allocation
    // Read from provided buffer (already mapped by caller)
    // Apply AWB/AE
    // Run ONNX
    // Write back to buffer (caller will munmap)
}
```

## File Structure
```
src/ipa/softisp/
├── softisp.h              # Class declaration
├── softisp.cpp            # Skeleton (includes methods)
├── SoftIsp_init.cpp       # Model loading
├── SoftIsp_start.cpp      # Start processing
├── SoftIsp_stop.cpp       # Stop processing
├── SoftIsp_configure.cpp  # Configure
├── SoftIsp_queueRequest.cpp
├── SoftIsp_computeParams.cpp
├── SoftIsp_processStats.cpp   # Stage 1: Stats → AWB/AE
├── SoftIsp_processFrame.cpp   # Stage 2: Bayer → RGB/YUV
├── SoftIsp_logPrefix.cpp
├── onnx_engine.cpp        # ONNX Runtime wrapper
└── softisp_module.cpp     # Module entry point
```

## License
LGPL-2.1-or-later
