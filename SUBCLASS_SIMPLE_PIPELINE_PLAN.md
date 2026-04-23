# Plan: Subclass Simple Pipeline for SoftISP Integration

**Date:** 2026-04-23  
**Branch:** `feature/softisp-virtual-decoupled`  
**Goal:** Create a new pipeline handler that inherits from SimplePipelineHandler and adds virtual camera support with SoftISP IPA integration.

---

## Architecture Overview

```
SimplePipelineHandler (libcamera base)
         ↑
         | inherits
         |
SoftISPSimplePipelineHandler (new subclass)
         |
         ├── VirtualCamera integration (test patterns)
         ├── SoftISP IPA integration (ONNX AWB, etc.)
         └── Real camera support (via Simple's infrastructure)
```

---

## Why Subclass Simple?

### Advantages
1. ✅ **Full real camera support** - Inherits BFS traversal, link config, format propagation
2. ✅ **Production-ready infrastructure** - Entity management, concurrency control, delayed controls
3. ✅ **Virtual camera fallback** - Add our virtual camera when no real sensors found
4. ✅ **SoftISP IPA integration** - Plug in our ONNX-based processing
5. ✅ **Best of both worlds** - Real cameras + virtual test patterns

### Challenges
1. ⚠️ **Complex inheritance** - Need to override key methods carefully
2. ⚠️ **Entity management** - Virtual cameras don't have media entities
3. ⚠️ **Buffer handling** - Virtual cameras use memfd, real cameras use V4L2
4. ⚠️ **Configuration** - Need to handle both paths uniformly

---

## Implementation Strategy

### Phase 1: Create Base Subclass Structure

**File:** `src/libcamera/pipeline/softisp/softisp_simple.h`

```cpp
class SoftISPSimplePipelineHandler : public SimplePipelineHandler {
public:
    SoftISPSimplePipelineHandler(CameraManager *manager);
    ~SoftISPSimplePipelineHandler();

    bool match(DeviceEnumerator *enumerator) override;

private:
    bool createVirtualCamera();
    bool isVirtualCameraSupported();
};
```

**File:** `src/libcamera/pipeline/softisp/softisp_simple.cpp`

- Implement constructor/destructor
- Override `match()` to check for real cameras first
- If no real cameras, create virtual camera
- Call parent methods for real camera handling

---

### Phase 2: Virtual Camera Integration

**Goal:** Make virtual camera appear as a valid "camera" to the Simple pipeline

#### Option A: Fake Media Entity
Create a fake `MediaDevice` with a virtual `MediaEntity` that mimics a camera sensor.

**Pros:**
- Simple pipeline treats it like a real camera
- Full infrastructure reuse

**Cons:**
- Complex to fake media graph properly
- May break assumptions in Simple pipeline

#### Option B: Override Key Methods
Override `generateConfiguration()`, `configure()`, `exportFrameBuffers()` to handle virtual camera case.

**Pros:**
- Cleaner separation
- No need to fake media entities

**Cons:**
- More code duplication
- Need to handle dual code paths

**Recommended:** Option B - Override methods

---

### Phase 3: SoftISP IPA Integration

**Current State:**
- SoftISP IPA is already implemented (`src/ipa/softisp/`)
- IPA proxy interface exists

**Integration Points:**

1. **In `configure()`**:
   ```cpp
   if (isVirtualCamera) {
       // Setup SoftISP IPA for virtual camera
       ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(...);
       ipa_->configure(...);
   } else {
       // Use Simple's normal configuration
       parent::configure(...);
   }
   ```

2. **In `processRequest()`** (via buffer callbacks):
   ```cpp
   void imageBufferReady(FrameBuffer *buffer) {
       if (isVirtualCamera) {
           // Call SoftISP IPA processFrame
           ipa_->processFrame(...);
       }
       // Let parent handle completion
       parent::imageBufferReady(buffer);
   }
   ```

---

### Phase 4: Buffer Handling for Virtual Camera

**Problem:** Simple pipeline expects V4L2 buffer export, virtual camera uses memfd

**Solution:** Override `exportFrameBuffers()` for virtual camera:

```cpp
int SoftISPSimplePipelineHandler::exportFrameBuffers(
    Camera *camera, Stream *stream,
    std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    if (isVirtualCamera(camera)) {
        // Use SoftISP's memfd-based buffer export
        return exportVirtualBuffers(stream, buffers);
    } else {
        // Use parent's V4L2 buffer export
        return SimplePipelineHandler::exportFrameBuffers(camera, stream, buffers);
    }
}
```

---

### Phase 5: Request Handling

**Problem:** Virtual camera processes synchronously, Simple expects async

**Solution:** Override `queueRequestDevice()`:

```cpp
int SoftISPSimplePipelineHandler::queueRequestDevice(
    Camera *camera, Request *request)
{
    if (isVirtualCamera(camera)) {
        // Process synchronously for virtual camera
        auto cameraData = cameraData(camera);
        cameraData->processRequest(request);
        return 0;
    } else {
        // Use parent's async handling
        return SimplePipelineHandler::queueRequestDevice(camera, request);
    }
}
```

---

## File Structure

```
src/libcamera/pipeline/softisp/
├── softisp_simple.h          # New: Subclass declaration
├── softisp_simple.cpp        # New: Subclass implementation
├── softisp.h                 # Existing: SoftISPCameraData (reuse)
├── softisp.cpp               # Existing: SoftISP logic (adapt)
└── virtual_camera.h/cpp      # Existing: Virtual camera (reuse)
```

---

## Key Methods to Override

| Method | Parent Behavior | Override Behavior |
|--------|----------------|-------------------|
| `match()` | Check supported devices | Check real cameras → fallback to virtual |
| `generateConfiguration()` | Sensor-based config | Virtual: fixed 1920x1080 NV12 |
| `configure()` | Link setup + format propagation | Virtual: IPA config only |
| `exportFrameBuffers()` | V4L2 buffer export | Virtual: memfd export |
| `start()` | V4L2 streamOn + entity setup | Virtual: VirtualCamera::start() |
| `stopDevice()` | V4L2 streamOff + cleanup | Virtual: VirtualCamera::stop() |
| `queueRequestDevice()` | Async V4L2 queue | Virtual: Sync process |

---

## Implementation Steps

### Step 1: Create Subclass Header
```bash
touch src/libcamera/pipeline/softisp/softisp_simple.h
```

### Step 2: Implement Basic Subclass
- Constructor/destructor
- `match()` override with fallback logic
- Helper methods for virtual camera detection

### Step 3: Add Virtual Camera Support
- `createVirtualCamera()` method
- Virtual camera configuration
- Buffer export override

### Step 4: Integrate SoftISP IPA
- IPA initialization in `configure()`
- Frame processing in request handling
- Stats/metadata handling

### Step 5: Test Real Camera Path
- Ensure parent's real camera support works
- No regression in Simple pipeline behavior

### Step 6: Test Virtual Camera Path
- Virtual camera enumeration
- Buffer allocation
- Frame generation
- IPA processing

### Step 7: Test Hybrid Scenario
- System with both real and virtual cameras
- Proper prioritization

---

## Code Skeleton

### softisp_simple.h
```cpp
#pragma once

#include "libcamera/internal/pipeline_handler.h"
#include "libcamera/internal/simple_pipeline_handler.h"
#include "softisp.h"
#include "virtual_camera.h"

namespace libcamera {

class SoftISPSimplePipelineHandler : public SimplePipelineHandler {
public:
    SoftISPSimplePipelineHandler(CameraManager *manager);
    ~SoftISPSimplePipelineHandler();

    std::unique_ptr<CameraConfiguration> generateConfiguration(
        Camera *camera, Span<const StreamRole> roles) override;
    int configure(Camera *camera, CameraConfiguration *config) override;
    int exportFrameBuffers(Camera *camera, Stream *stream,
                          std::vector<std::unique_ptr<FrameBuffer>> *buffers) override;
    int start(Camera *camera, const ControlList *controls) override;
    void stopDevice(Camera *camera) override;
    bool match(DeviceEnumerator *enumerator) override;

private:
    bool isVirtualCamera(Camera *camera);
    bool createVirtualCamera();
    int exportVirtualBuffers(Stream *stream,
                            std::vector<std::unique_ptr<FrameBuffer>> *buffers);
    void processVirtualRequest(Request *request);

    std::set<Camera *> virtualCameras_;
    Mutex virtualCamerasMutex_;
};

} // namespace libcamera
```

### softisp_simple.cpp (partial)
```cpp
#include "softisp_simple.h"

LOG_DEFINE_CATEGORY(SoftISPSimplePipeline)

SoftISPSimplePipelineHandler::SoftISPSimplePipelineHandler(CameraManager *manager)
    : SimplePipelineHandler(manager)
{
    LOG(SoftISPSimplePipeline, Info) << "SoftISP Simple pipeline handler created";
}

bool SoftISPSimplePipelineHandler::match(DeviceEnumerator *enumerator)
{
    LOG(SoftISPSimplePipeline, Info) << "Matching cameras";

    // First, try to match real cameras using parent's logic
    bool realCamerasMatched = false;
    
    // ... parent's match logic for real cameras ...
    
    // If no real cameras found, create virtual camera
    if (!realCamerasMatched) {
        LOG(SoftISPSimplePipeline, Info) << "No real cameras found, creating virtual camera";
        return createVirtualCamera();
    }

    return realCamerasMatched;
}

bool SoftISPSimplePipelineHandler::createVirtualCamera()
{
    // Create virtual camera data
    auto cameraData = std::make_unique<SoftISPCameraData>(this);
    cameraData->isVirtualCamera = true;
    
    if (cameraData->init() < 0)
        return false;

    // Generate configuration
    auto config = cameraData->generateConfiguration({ StreamRole::Viewfinder });
    if (!config || config->validate() == CameraConfiguration::Invalid)
        return false;

    // Register camera
    if (!registerCamera(std::move(cameraData), *config))
        return false;

    return true;
}
```

---

## Testing Plan

### Test 1: Virtual Camera Only (Termux)
```bash
libcamera-hello --list-cameras
# Expected: "SoftISP Virtual Camera"

libcamera-vid --timeout 5000 -o test.264
# Expected: Captures test pattern frames
```

### Test 2: Real Camera Only (Embedded Device)
```bash
libcamera-hello --list-cameras
# Expected: Real camera sensor (e.g., "imx219")

libcamera-vid --timeout 5000 -o test.264
# Expected: Captures from real sensor
```

### Test 3: Both Real and Virtual
```bash
# Simulate both (requires custom setup)
libcamera-hello --list-cameras
# Expected: Real camera listed first, virtual as fallback

libcamera-hello --camera 0
# Expected: Uses real camera

libcamera-hello --camera 1
# Expected: Uses virtual camera
```

### Test 4: IPA Processing
```bash
# Verify SoftISP IPA is called
export LIBCAMERA_LOG_LEVELS="*:Warn,SoftISPSimplePipeline:Debug"
libcamera-hello --timeout 5000
# Expected: Logs show IPA processStats/processFrame calls
```

---

## Potential Issues & Solutions

### Issue 1: Parent's match() Returns Early
**Problem:** Simple's match() might return false before we can add virtual camera

**Solution:** Override match() completely, call parent methods selectively

### Issue 2: Entity Requirements
**Problem:** Simple pipeline expects MediaEntity for all cameras

**Solution:** Virtual camera bypasses entity requirements by overriding key methods

### Issue 3: Buffer Callbacks
**Problem:** Simple expects V4L2 bufferReady signals

**Solution:** Virtual camera manually completes requests (no callbacks needed)

### Issue 4: Configuration Validation
**Problem:** Simple's configuration expects sensor properties

**Solution:** Virtual camera provides mock sensor data or bypasses validation

---

## Next Steps

1. ✅ **Commit current changes** (Done)
2. ⏳ **Create softisp_simple.h/cpp skeleton**
3. ⏳ **Implement match() override**
4. ⏳ **Add virtual camera creation**
5. ⏳ **Override buffer export**
6. ⏳ **Override request handling**
7. ⏳ **Integrate SoftISP IPA**
8. ⏳ **Test virtual camera path**
9. ⏳ **Test real camera path**
10. ⏳ **Fix issues and refine**

---

## Estimated Timeline

- **Phase 1 (Skeleton):** 1 day
- **Phase 2 (Virtual Camera):** 2 days
- **Phase 3 (IPA Integration):** 2 days
- **Phase 4 (Testing & Debugging):** 3 days
- **Phase 5 (Polish & Documentation):** 1 day

**Total:** ~9 days (1.5 weeks)

---

## Conclusion

Subclassing SimplePipelineHandler is the **right approach** because:

1. ✅ Reuses production-ready real camera infrastructure
2. ✅ Adds virtual camera capability cleanly
3. ✅ Integrates SoftISP IPA naturally
4. ✅ Maintains compatibility with libcamera ecosystem
5. ✅ Provides best user experience (real or virtual)

**Ready to implement!**

---

*Generated: 2026-04-23*  
*Branch: feature/softisp-virtual-decoupled*  
*Status: Ready for implementation*
