# SoftISP Async Architecture

## Buffer Lifecycle with memfd

### Key Insight
**No explicit `mmap`/`munmap` needed!** The `memfd` approach simplifies everything:

1. **Pipeline creates buffer**: `memfd_create()` → `FrameBuffer` wraps FD in `SharedFD`
2. **FD stays open**: `SharedFD` keeps FD alive as long as `FrameBuffer` exists
3. **IPA receives FD**: IPA can `mmap` internally (or use direct FD access)
4. **IPA processes async**: Keeps FD open during processing
5. **Callback signals done**: `metadataReady` tells Pipeline it's safe
6. **Pipeline completes**: `completeRequest()` → `FrameBuffer` destroyed → FD closed → memory freed

### Why memfd is Perfect
- ✅ **Automatic cleanup**: When FD closed, memory freed
- ✅ **No mapping management**: Kernel handles it
- ✅ **Safe sharing**: FD can be passed between threads/processes
- ✅ **No leaks**: `SharedFD` ensures FD stays open until done
- ✅ **Simple code**: No manual `mmap`/`munmap` calls

## Async Processing Flow

```
┌─────────────────────────────────────────────────────────────────────┐
│  Pipeline (Caller)                                                  │
│                                                                     │
│  1. Create FrameBuffer with memfd                                   │
│     └─> FD managed by SharedFD (auto cleanup)                       │
│                                                                     │
│  2. Pass FD to IPA (ipa_->processStats/Frame)                       │
│     └─> SharedFD keeps FD alive                                     │
│                                                                     │
│  3. Store request in pendingRequests_                               │
│     └─> Track frameId → FrameInfo mapping                           │
│                                                                     │
│  4. Return (don't complete yet!)                                    │
│     └─> IPA is still processing                                     │
└───────────────────────┬─────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────────┐
│  IPA (Callee) - Stateless                                           │
│                                                                     │
│  1. Receive FD                                                      │
│     └─> Map internally if needed (mmap)                             │
│                                                                     │
│  2. Process data (async)                                            │
│     └─> Run ONNX inference                                          │
│     └─> Keep FD open during processing                              │
│                                                                     │
│  3. Unmap (if mapped)                                               │
│     └─> Still keep FD open                                          │
│                                                                     │
│  4. Signal completion                                               │
│     └─> metadataReady.emit(frameId, metadata)                       │
└───────────────────────┬─────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────────────┐
│  Pipeline (Caller) - Callback Handler                               │
│                                                                     │
│  1. metadataReady(frameId, metadata)                                │
│     └─> Find FrameInfo in pendingRequests_                          │
│     └─> Merge metadata into request                                 │
│     └─> Mark statsReady = true                                      │
│                                                                     │
│  2. Check if frameReady also true                                   │
│     └─> Yes: pendingRequests_.erase(frameId)                        │
│         → pipe()->completeRequest(request)                          │
│         → FrameBuffer destroyed                                     │
│         → SharedFD closes FD                                        │
│         → memfd memory freed automatically                          │
│                                                                     │
│     └─> No: Wait for frame callback (if separate)                   │
└─────────────────────────────────────────────────────────────────────┘
```

## Code Example

### Pipeline (Caller)
```cpp
void SoftISPCameraData::processRequest(Request *request) {
    uint32_t frameId = nextFrameId++;
    
    // Track pending request
    auto info = std::make_unique<SoftISPFrameInfo>(this, request, frameId);
    pendingRequests_[frameId] = std::move(info);
    
    for (const auto &bufferPair : buffers) {
        FrameBuffer *fb = bufferPair.second;
        const FrameBuffer::Plane &plane = fb->planes()[0];
        SharedFD fd = plane.fd; // FD stays open!
        
        // Call IPA (async)
        ipa_->processStats(frameId, 0, ControlList());
        ipa_->processFrame(frameId, 0, fd, 0, width, height, ControlList());
        
        // Don't complete yet! Wait for callback.
        // info->frameReady = true;
    }
}

void SoftISPCameraData::metadataReady(unsigned int frameId, const ControlList &metadata) {
    auto it = pendingRequests_.find(frameId);
    if (it == pendingRequests_.end()) return;
    
    SoftISPFrameInfo *info = it->second.get();
    info->request->_d()->metadata().merge(metadata);
    info->statsReady = true;
    
    if (info->statsReady && info->frameReady) {
        pendingRequests_.erase(it);
        pipe()->completeRequest(info->request);
        // FrameBuffer destroyed → FD closed → memfd freed
    }
}
```

### IPA (Callee)
```cpp
void SoftIsp::processStats(uint32_t frameId, uint32_t bufferId, 
                           const ControlList &controls) {
    // Receive FD from caller
    // Map internally if needed (optional)
    void *data = mmap(..., fd, ...);
    
    // Process (async)
    // ...
    
    // Unmap
    munmap(data, size);
    // FD still open (caller owns it)
    
    // Signal completion
    metadataReady.emit(frameId, metadata);
}
```

## Benefits of This Pattern

### 1. No Memory Leaks
- `SharedFD` ensures FD stays open
- FD closed when `FrameBuffer` destroyed
- `memfd` memory freed automatically

### 2. Thread Safe
- `SharedFD` uses atomic ref counting
- FD valid across threads
- No race conditions

### 3. Simple Code
- No manual `mmap`/`munmap` in Pipeline
- No buffer tracking complexity
- Clear ownership (Pipeline owns FD)

### 4. Async Ready
- IPA can process in background
- Callback signals completion
- Pipeline knows when safe to reuse buffer

### 5. Matches libcamera Pattern
- Same as rkisp1, ipu3, etc.
- Uses `metadataReady` callback
- Follows libcamera conventions

## Comparison

### ❌ Wrong (Synchronous/Blocking)
```cpp
// DON'T DO THIS:
ipa_->processStats(...);  // Blocks?
ipa_->processFrame(...);  // Blocks?
munmap(data, size);       // Too early! IPA might still use it
```

### ✅ Right (Async with Callback)
```cpp
// DO THIS:
ipa_->processStats(...);  // Returns immediately
ipa_->processFrame(...);  // Returns immediately
// Don't munmap! Wait for callback...
// In metadataReady():
//   pendingRequests_.erase(it);
//   completeRequest();  // FrameBuffer destroyed → FD closed → memfd freed
```

## License
LGPL-2.1-or-later
