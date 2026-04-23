# Refactoring dummysoftisp to Use VirtualCameraStub

## Current Implementation (Before)

### File: `src/libcamera/pipeline/dummysoftisp/softisp.h`
```cpp
class DummySoftISPCameraData : public Camera::Private, public Thread {
public:
    DummySoftISPCameraData(PipelineHandlerDummysoftisp *pipe);
    ~DummySoftISPCameraData();
    int init();
    int loadIPA();
    void run() override;  // Thread loop
    void processRequest(Request *request);
    
    // Members
    std::unique_ptr<ipa::soft::IPAProxySoft> ipa_;
    std::vector<StreamConfig> streamConfigs_;
    bool running_ = false;
    Mutex mutex_;
    std::map<uint32_t, FrameBuffer*> bufferMap_;
};
```

### File: `src/libcamera/pipeline/dummysoftisp/softisp.cpp`
```cpp
// ~100 lines of thread management, request queuing, buffer handling
void DummySoftISPCameraData::run() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // Process requests...
    }
}

void DummySoftISPCameraData::processRequest(Request *request) {
    // Extract buffers, call IPA, complete request
}
```

---

## Refactored Implementation (After)

### File: `src/libcamera/pipeline/dummysoftisp/softisp.h`
```cpp
#pragma once

#include <map>
#include <memory>
#include <vector>

#include <libcamera/camera.h>
#include "libcamera/internal/camera.h"
#include "libcamera/internal/pipeline_handler.h"
#include "libcamera/internal/virtual_camera_stub.h"  // NEW!
#include <libcamera/ipa/soft_ipa_interface.h>
#include <libcamera/ipa/soft_ipa_proxy.h>

namespace libcamera {

class PipelineHandlerDummysoftisp;

/**
 * @brief Camera data for SoftISP dummy pipeline
 * 
 * Uses VirtualCameraStub for thread management and request processing.
 * Only implements SoftISP-specific logic.
 */
class DummySoftISPCameraData : public Camera::Private, public VirtualCameraStub {
public:
    DummySoftISPCameraData(PipelineHandlerDummysoftisp *pipe);
    ~DummySoftISPCameraData();

    int init();
    int loadIPA();

    // Buffer management (still needed for IPA integration)
    FrameBuffer* getBufferFromId(uint32_t bufferId);
    void storeBuffer(uint32_t bufferId, FrameBuffer *buffer);

protected:
    /**
     * @brief Process request with SoftISP IPA
     * @param request Request to process
     * 
     * Override to add SoftISP-specific processing before/after stub's default
     */
    void processRequest(Request *request) override;

    /**
     * @brief Generate frame (optional - can use stub's default)
     */
    int generateFrame(FrameBuffer *buffer, uint32_t sequence) override;

private:
    std::unique_ptr<ipa::soft::IPAProxySoft> ipa_;
    std::vector<StreamConfig> streamConfigs_;
    std::map<uint32_t, FrameBuffer*> bufferMap_;
    
    struct StreamConfig {
        Stream *stream = nullptr;
        unsigned int seq = 0;
    };
};

/**
 * @brief Pipeline handler for SoftISP with dummy cameras
 */
class PipelineHandlerDummysoftisp : public PipelineHandler {
public:
    static bool created_;
    
    PipelineHandlerDummysoftisp(CameraManager *manager);
    ~PipelineHandlerDummysoftisp();

    std::unique_ptr<CameraConfiguration> generateConfiguration(
        Camera *camera, Span<const StreamRole> roles) override;
    int configure(Camera *camera, CameraConfiguration *config) override;
    int exportFrameBuffers(Camera *camera, Stream *stream,
                          std::vector<std::unique_ptr<FrameBuffer>> *buffers) override;
    int start(Camera *camera, const ControlList *controls) override;
    void stopDevice(Camera *camera) override;
    int queueRequestDevice(Camera *camera, Request *request) override;
    bool match(DeviceEnumerator *enumerator) override;

private:
    DummySoftISPCameraData *cameraData(Camera *camera) {
        return static_cast<DummySoftISPCameraData *>(camera->_d());
    }
};

} // namespace libcamera
```

### File: `src/libcamera/pipeline/dummysoftisp/softisp.cpp` (Simplified)

```cpp
#include "softisp.h"

#include <libcamera/base/log.h>
#include <libcamera/controls.h>
#include <libcamera/property_ids.h>

#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPPipeline)

/* -----------------------------------------------------------------------------
 * DummySoftISPCameraData Implementation
 * ---------------------------------------------------------------------------*/

DummySoftISPCameraData::DummySoftISPCameraData(PipelineHandlerDummysoftisp *pipe)
    : Camera::Private(pipe), VirtualCameraStub(pipe)
{
    LOG(SoftISPPipeline, Debug) << "Creating SoftISP dummy camera";
}

DummySoftISPCameraData::~DummySoftISPCameraData()
{
    stop();  // VirtualCameraStub::stop()
}

int DummySoftISPCameraData::init()
{
    LOG(SoftISPPipeline, Info) << "Initializing SoftISP dummy camera";
    return 0;
}

int DummySoftISPCameraData::loadIPA()
{
    ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoft>(pipe(), 0, 0);
    if (!ipa_) {
        LOG(SoftISPPipeline, Error) << "Failed to create SoftISP IPA module";
        return -ENOENT;
    }
    
    LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded";
    return 0;
}

void DummySoftISPCameraData::processRequest(Request *request)
{
    // Get buffer from request
    const auto &buffers = request->buffers();
    if (buffers.empty()) {
        request->complete(-EINVAL);
        return;
    }

    FrameBuffer *buffer = buffers[0].buffer;
    uint32_t bufferId = buffers[0].planeIndexes[0];
    
    // Store buffer mapping for IPA
    storeBuffer(bufferId, buffer);

    // Call IPA to process statistics
    if (ipa_) {
        int ret = ipa_->processStats(sequence(), bufferId, 
                                     request->controls());
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "IPA processStats failed: " << ret;
        }
    }

    // Let stub complete the request
    VirtualCameraStub::processRequest(request);
}

int DummySoftISPCameraData::generateFrame(FrameBuffer *buffer, 
                                          uint32_t sequence)
{
    // Optional: Generate test pattern or synthetic data
    // For now, just return success (buffer unchanged)
    return 0;
}

FrameBuffer* DummySoftISPCameraData::getBufferFromId(uint32_t bufferId)
{
    auto it = bufferMap_.find(bufferId);
    return (it != bufferMap_.end()) ? it->second : nullptr;
}

void DummySoftISPCameraData::storeBuffer(uint32_t bufferId, 
                                         FrameBuffer *buffer)
{
    bufferMap_[bufferId] = buffer;
}

/* -----------------------------------------------------------------------------
 * PipelineHandlerDummysoftisp Implementation
 * ---------------------------------------------------------------------------*/

bool PipelineHandlerDummysoftisp::created_ = false;

PipelineHandlerDummysoftisp::PipelineHandlerDummysoftisp(CameraManager *manager)
    : PipelineHandler(manager)
{
    LOG(SoftISPPipeline, Info) << "SoftISP dummy pipeline handler created";
}

PipelineHandlerDummysoftisp::~PipelineHandlerDummysoftisp()
{
    LOG(SoftISPPipeline, Info) << "SoftISP dummy pipeline handler destroyed";
}

bool PipelineHandlerDummysoftisp::match(DeviceEnumerator *enumerator)
{
    // Create a virtual camera (no hardware needed)
    auto cameraData = std::make_unique<DummySoftISPCameraData>(this);
    
    if (cameraData->init() < 0)
        return false;

    if (cameraData->loadIPA() < 0)
        return false;

    // Create camera with a single stream
    std::vector<StreamRole> roles = { StreamRole::Viewfinder };
    auto config = cameraData->generateConfiguration(roles);
    if (!config || config->validate() == CameraConfiguration::Invalid)
        return false;

    // Register camera
    CameraConfiguration &cameraConfig = *config;
    Stream *stream = cameraConfig.at(0).stream();
    
    std::unique_ptr<Camera> camera = registerCamera(
        std::move(cameraData), cameraConfig);
    
    if (!camera)
        return false;

    LOG(SoftISPPipeline, Info) << "SoftISP dummy camera registered";
    return true;
}

std::unique_ptr<CameraConfiguration> 
PipelineHandlerDummysoftisp::generateConfiguration(
    Camera *camera, Span<const StreamRole> roles)
{
    auto cameraData = cameraData(camera);
    
    auto config = std::make_unique<CameraConfiguration>();
    
    // Default configuration: 640x480 NV12
    StreamConfiguration cfg;
    cfg.size = Size(640, 480);
    cfg.pixelFormat = formats::NV12;
    cfg.bufferCount = 4;
    
    config->addConfiguration(cfg);
    return config;
}

int PipelineHandlerDummysoftisp::configure(
    Camera *camera, CameraConfiguration *config)
{
    auto cameraData = cameraData(camera);
    
    // Store stream reference
    cameraData->streamConfigs_.resize(1);
    cameraData->streamConfigs_[0].stream = config->at(0).stream();
    
    return 0;
}

int PipelineHandlerDummysoftisp::exportFrameBuffers(
    Camera *camera, Stream *stream,
    std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    auto cameraData = cameraData(camera);
    
    // Allocate buffers using DMA allocator or memfd
    // This is pipeline-specific, so keep it here
    // (VirtualCameraStub doesn't handle buffer allocation)
    
    // Example: Use simple allocation
    for (unsigned int i = 0; i < 4; ++i) {
        int fd = memfd_create("dummysoftisp_buffer", MFD_CLOEXEC);
        if (fd < 0)
            return -errno;
        
        size_t size = 640 * 480 * 1.5; // NV12 size
        if (ftruncate(fd, size) < 0) {
            close(fd);
            return -errno;
        }
        
        auto buffer = std::make_unique<FrameBuffer>();
        buffer->planes().emplace_back(FrameBuffer::Plane{fd, size});
        fd.release();
        
        buffers->push_back(std::move(buffer));
    }
    
    return 0;
}

int PipelineHandlerDummysoftisp::start(
    Camera *camera, const ControlList *controls)
{
    auto cameraData = cameraData(camera);
    return cameraData->start();  // VirtualCameraStub::start()
}

void PipelineHandlerDummysoftisp::stopDevice(Camera *camera)
{
    auto cameraData = cameraData(camera);
    cameraData->stop();  // VirtualCameraStub::stop()
}

int PipelineHandlerDummysoftisp::queueRequestDevice(
    Camera *camera, Request *request)
{
    auto cameraData = cameraData(camera);
    cameraData->queueRequest(request);  // VirtualCameraStub::queueRequest()
    return 0;
}

} // namespace libcamera
```

---

## Benefits of This Refactoring

### Code Reduction
- **Before**: ~100 lines of thread/request handling
- **After**: ~30 lines of SoftISP-specific logic
- **Reduction**: 70% less code in `DummySoftISPCameraData`

### Separation of Concerns
- **VirtualCameraStub**: Thread management, request queue, basic processing
- **DummySoftISPCameraData**: SoftISP IPA integration, buffer mapping

### Reusability
- Same `VirtualCameraStub` can be used by other dummy pipelines
- Easy to create new virtual cameras by inheriting from stub

### Maintainability
- Bug fixes in `VirtualCameraStub` benefit all pipelines
- Clear interface for extending functionality

---

## Migration Steps

1. **Create stub file** (already done):
   ```bash
   src/libcamera/internal/virtual_camera_stub.h
   ```

2. **Update header** (`softisp.h`):
   - Add `#include "libcamera/internal/virtual_camera_stub.h"`
   - Change inheritance: `public Thread` → `public VirtualCameraStub`
   - Remove `run()` declaration
   - Add `processRequest()` and `generateFrame()` overrides

3. **Update implementation** (`softisp.cpp`):
   - Remove `run()` method (handled by stub)
   - Simplify `processRequest()` to call stub's method
   - Add `generateFrame()` override if needed
   - Keep IPA-specific logic

4. **Update pipeline methods**:
   - `start()` → call `cameraData->start()`
   - `stopDevice()` → call `cameraData->stop()`
   - `queueRequestDevice()` → call `cameraData->queueRequest()`

5. **Test**:
   ```bash
   meson compile -C build
   ./build/tools/softisp-test-app --pipeline dummysoftisp --frames 5
   ```

---

## Comparison: Before vs After

| Feature | Before | After |
|---------|--------|-------|
| **Lines of Code** | ~100 | ~30 |
| **Thread Management** | Manual | Automatic (stub) |
| **Request Queue** | Manual | Automatic (stub) |
| **Buffer Mapping** | Manual | Manual (kept) |
| **IPA Integration** | Manual | Manual (kept) |
| **Reusability** | None | High (stub reusable) |
| **Maintainability** | Medium | High |

---

## Conclusion

Using `VirtualCameraStub` simplifies `dummysoftisp` significantly while maintaining all SoftISP-specific functionality. The stub handles the boilerplate (thread, queue, timing), allowing you to focus on the AI processing logic.

**Ready to implement?** I can create the refactored files for you.
