# SoftISP IPA Module

## Architecture: Caller/Callee Pattern

### Design Principle
- **Caller (Pipeline)**: Pre-calculates all buffer sizes, maps memory, prepares resources
- **Callee (IPA)**: Stateless processing unit, only reads/writes data

### Benefits
- **Stateless IPA**: Easier to test, no hidden state, thread-safe
- **Clear Separation**: Pipeline owns memory management, IPA owns processing
- **Performance**: Buffers pre-mapped, no allocation during processing
- **Debugging**: Clear boundary between resource management and processing

## Two-Stage Processing

### Stage 1: `processStats()` (Callee)
**Caller (Pipeline) Preparation:**
1. Calculate stats buffer size based on sensor format
2. Map stats buffer via `mmap()`
3. Populate stats buffer with sensor data

**Callee (IPA) Processing:**
1. Read stats data from pre-mapped buffer
2. Run `algo.onnx` inference
3. Extract AWB/AE parameters
4. Emit metadata via `metadataReady` signal

**IPA is stateless:** No internal state between calls.

### Stage 2: `processFrame()` (Callee)
**Caller (Pipeline) Preparation:**
1. Calculate frame buffer size (width × height × bytes_per_pixel)
2. Map frame buffer via `mmap()`
3. Ensure AWB/AE metadata from Stage 1 is available

**Callee (IPA) Processing:**
1. Read Bayer frame from pre-mapped buffer
2. Apply AWB/AE parameters
3. Run `applier.onnx` inference
4. Convert Bayer → RGB/YUV
5. Write processed data back to pre-mapped buffer

**IPA is stateless:** No internal state between calls.

## Data Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    CALLER (Pipeline)                        │
│  1. Calculate buffer sizes (width, height, stride)          │
│  2. Allocate buffers (FrameBuffer)                          │
│  3. Map buffers (mmap)                                      │
│  4. Populate with sensor data                               │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                    CALLEE (IPA)                             │
│  Stateless Processing:                                      │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │ processStats │ →  │ algo.onnx    │ →  │ metadata     │  │
│  │ (read stats) │    │ (AWB/AE)     │    │ (emit)       │  │
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
// 1. Pre-calculate sizes
uint32_t frameSize = width * height * bytes_per_pixel;
uint32_t statsSize = calculateStatsSize(sensorFormat);

// 2. Allocate and map
void *frameData = mmap(nullptr, frameSize, PROT_READ | PROT_WRITE, ...);
void *statsData = mmap(nullptr, statsSize, PROT_READ | PROT_WRITE, ...);

// 3. Populate data
populateStats(statsData, sensorData);

// 4. Call IPA (stateless)
ipa_->processStats(frameId, bufferId, metadata);
ipa_->processFrame(frameId, bufferId, fd, planeIndex, width, height, results);

// 5. Cleanup
munmap(frameData, frameSize);
munmap(statsData, statsSize);
```

### For IPA (Callee)
```cpp
void SoftIsp::processStats(uint32_t frame, uint32_t bufferId,
                           const ControlList &sensorControls)
{
    // NO state maintained
    // Just read from provided buffers, process, emit metadata
    
    // 1. Read stats (caller already mapped)
    // 2. Run ONNX
    // 3. Emit metadata
    metadataReady.emit(frame, metadata);
}

void SoftIsp::processFrame(uint32_t frame, uint32_t bufferId,
                           const SharedFD &bufferFd, ...)
{
    // NO state maintained
    // Just read from provided buffers, process, write back
    
    // 1. Read Bayer (caller already mapped)
    // 2. Apply AWB/AE (from previous processStats)
    // 3. Run ONNX
    // 4. Write back to buffer (caller will munmap)
}
```

## File Structure
```
src/ipa/softisp/
├── softisp.h              # Class declaration
├── softisp.cpp            # Stateless method implementations
├── onnx_engine.h/cpp      # ONNX Runtime wrapper
├── softisp_module.cpp     # Module entry point
└── README.md              # This file
```

## License
LGPL-2.1-or-later
