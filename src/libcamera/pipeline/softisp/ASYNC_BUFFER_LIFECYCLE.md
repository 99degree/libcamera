# SoftISP Async Buffer Lifecycle

## Problem Statement

In an asynchronous ISP pipeline, the Pipeline (Caller) needs to know when the IPA (Callee) is done processing buffers so it can:
1. Safely reuse the **source buffer** (Bayer input)
2. Safely free/reuse the **output buffer** (RGB/YUV result)
3. Complete the request to the application

Without proper callbacks, the Pipeline might free buffers while the IPA is still using them, causing crashes or data corruption.

## Solution: Dual-Callback Pattern (matches rkisp1/ipu3)

### Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────┐
│  Application                                                          │
│  - Requests frames                                                    │
│  - Receives completed frames                                          │
│  - Reuses/frees buffers after completion                              │
└───────────────────────┬─────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Pipeline (Caller)                                                    │
│                                                                       │
│  1. Create buffers (memfd)                                            │
│     - Source buffer (Bayer input)                                     │
│     - Output buffer (RGB/YUV)                                         │
│                                                                       │
│  2. Track request state:                                              │
│     - metadataReceived_ = false                                       │
│     - frameReceived_ = false                                          │
│                                                                       │
│  3. Call IPA (async)                                                  │
│     - ipa_->processStats(frameId, ...)                                │
│     - ipa_->processFrame(frameId, sourceFd, outputFd, ...)            │
│                                                                       │
│  4. Wait for callbacks...                                             │
│                                                                       │
│  5. metadataReady(frameId, metadata)                                  │
│     - Merge metadata into request                                     │
│     - metadataReceived_ = true                                        │
│     - Check if frameReceived_ also true                               │
│                                                                       │
│  6. frameDone(frameId, bufferId)                                      │
│     - frameReceived_ = true                                           │
│     - Check if metadataReceived_ also true                            │
│                                                                       │
│  7. Both received? → Complete request                                 │
│     - pipe()->completeRequest(request)                                │
│     - FrameBuffer destroyed → FD closed → memfd freed                 │
└───────────────────────┬─────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────────┐
│  IPA (Callee) - Stateless                                             │
│                                                                       │
│  1. Receive FD references (SharedFD keeps them alive)                 │
│     - sourceFd (Bayer input)                                          │
│     - outputFd (RGB/YUV output)                                       │
│                                                                       │
│  2. Process asynchronously                                            │
│     - Map buffers internally (if needed)                              │
│     - Run ONNX inference                                              │
│     - Write results to output buffer                                  │
│                                                                       │
│  3. Signal metadataReady()                                            │
│     - AWB/AE parameters computed                                      │
│     - Pipeline merges metadata                                        │
│                                                                       │
│  4. Signal frameDone()                                                │
│     - Frame processing complete                                       │
│     - Output buffer written                                           │
│     - Pipeline can now free buffers                                   │
└─────────────────────────────────────────────────────────────────────┘
```

## Callback Details

### 1. metadataReady(frameId, metadata)

**Purpose**: Signal that stats processing is complete and AWB/AE parameters are ready.

**When called**: After `processStats()` finishes ONNX inference.

**What Pipeline does**:
```cpp
void SoftISPCameraData::metadataReady(unsigned int frame, const ControlList &metadata) {
    SoftISPFrameInfo *info = frameInfo_.find(frame);
    if (!info) return;
    
    // Merge metadata into request
    info->request->_d()->metadata().merge(metadata);
    info->metadataReceived = true;
    
    // Check if frame is also done
    if (info->metadataReceived && info->frameReceived) {
        tryCompleteRequest(info);
    }
}
```

### 2. frameDone(frameId, bufferId)

**Purpose**: Signal that frame processing is complete and output buffer is ready.

**When called**: After `processFrame()` finishes ONNX inference and writes output.

**What Pipeline does**:
```cpp
void SoftISPCameraData::frameDone(unsigned int frame, unsigned int bufferId) {
    SoftISPFrameInfo *info = frameInfo_.find(frame);
    if (!info) return;
    
    info->frameReceived = true;
    
    // Check if metadata is also ready
    if (info->metadataReceived && info->frameReceived) {
        tryCompleteRequest(info);
    }
}
```

## Buffer Lifecycle

### Source Buffer (Bayer Input)

1. **Created**: Pipeline creates with `MemFd::create()`
2. **Mapped**: FrameBuffer wraps FD in `SharedFD`
3. **Passed**: Pipeline passes FD to `ipa_->processFrame()`
4. **Used**: IPA reads Bayer data (may mmap internally)
5. **Done**: IPA signals `frameDone()`
6. **Freed**: Pipeline completes request → FrameBuffer destroyed → FD closed → memfd freed

### Output Buffer (RGB/YUV)

1. **Created**: Pipeline creates with `MemFd::create()`
2. **Mapped**: FrameBuffer wraps FD in `SharedFD`
3. **Passed**: Pipeline passes FD to `ipa_->processFrame()`
4. **Used**: IPA writes RGB/YUV data (may mmap internally)
5. **Done**: IPA signals `frameDone()`
6. **Freed**: Pipeline completes request → FrameBuffer destroyed → FD closed → memfd freed

## Why memfd + SharedFD?

### memfd Benefits
- **Automatic cleanup**: When FD closed, memory freed automatically
- **No mapping management**: Kernel handles mmap/munmap
- **Safe sharing**: FD can be passed between threads/processes
- **No leaks**: No manual memory management needed

### SharedFD Benefits
- **Reference counting**: FD stays open as long as anyone needs it
- **Thread-safe**: Atomic ref counting
- **Automatic cleanup**: FD closed when last reference dropped
- **No race conditions**: Safe to pass between Pipeline and IPA

## Comparison with rkisp1/ipu3

### rkisp1 Pattern
```cpp
// rkisp1 uses hardware buffer-ready callbacks
stat_->bufferReady.connect(this, &RkISP1CameraData::statBufferReady);
mainPath_.bufferReady().connect(this, &PipelineHandlerRkISP1::imageBufferReady);

void RkISP1CameraData::statBufferReady(FrameBuffer *buffer) {
    // Stats buffer done
    info->statDequeued = true;
    tryCompleteRequest(info);
}

void PipelineHandlerRkISP1::imageBufferReady(FrameBuffer *buffer) {
    // Image buffer done
    tryCompleteRequest(info);
}
```

### SoftISP Pattern (matches rkisp1)
```cpp
// SoftISP uses IPA callbacks (same concept, different source)
ipa_->metadataReady.connect(this, &SoftISPCameraData::metadataReady);
ipa_->frameDone.connect(this, &SoftISPCameraData::frameDone);

void SoftISPCameraData::metadataReady(unsigned int frame, const ControlList &metadata) {
    // Stats/metadata done
    info->metadataReceived = true;
    tryCompleteRequest(info);
}

void SoftISPCameraData::frameDone(unsigned int frame, unsigned int bufferId) {
    // Frame processing done
    info->frameReceived = true;
    tryCompleteRequest(info);
}
```

## App Usage

### Application Code
```cpp
// 1. Create request with both source and output buffers
Request *request = camera->createRequest();
request->addBuffer(sourceStream, sourceBuffer);  // Bayer input
request->addBuffer(outputStream, outputBuffer);  // RGB/YUV output

// 2. Queue request
camera->queueRequest(request);

// 3. Wait for completion (via signal or poll)
// ...

// 4. Request completed - both buffers are now safe to reuse
// The Pipeline has already freed them (memfd auto-cleanup)
// App can create new buffers or reuse existing pool
```

## Implementation Checklist

### IPA Side
- [x] Add `frameDone` signal to `IPASoftIspInterface`
- [x] Emit `metadataReady()` in `processStats()`
- [x] Emit `frameDone()` in `processFrame()`
- [x] Keep FDs alive during processing (SharedFD)
- [x] Unmap buffers before signaling (if mapped)

### Pipeline Side
- [x] Track `metadataReceived` and `frameReceived` flags
- [x] Connect to `metadataReady` callback
- [x] Connect to `frameDone` callback
- [x] Complete request when BOTH callbacks received
- [x] Use memfd for buffer creation (auto-cleanup)
- [x] Pass FDs to IPA (SharedFD keeps alive)

### Benefits
✅ **No memory leaks**: memfd + SharedFD ensures cleanup  
✅ **Thread-safe**: Atomic ref counting, no race conditions  
✅ **No crashes**: Buffers not freed while IPA using them  
✅ **Matches rkisp1/ipu3**: Proven pattern, well-tested  
✅ **Simple code**: No manual mmap/munmap needed  
✅ **Production-ready**: Used in real libcamera pipelines  

## License
LGPL-2.1-or-later
