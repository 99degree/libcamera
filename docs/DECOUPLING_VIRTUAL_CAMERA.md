# Decoupling Virtual Camera as a Standalone Class

## Current State Analysis

### Existing Patterns in libcamera

After searching all pipeline handlers, here are the common patterns:

| Pipeline | CameraData Class | Inheritance | Complexity | Lines |
|----------|------------------|-------------|------------|-------|
| **virtual** | `VirtualCameraData` | `Camera::Private`, `Thread`, `Object` | High (config parsing, frame generation) | ~460 |
| **vimc** | `VimcCameraData` | `Camera::Private` | Medium (V4L2 devices, IPA) | ~400 |
| **simple** | `SimpleCameraData` | `Camera::Private` | Medium (sensor traversal) | ~500 |
| **dummysoftisp** | `DummySoftISPCameraData` | `Camera::Private`, `Thread` | Low (basic buffer handling) | ~100 |

### Key Finding

**All pipelines use the same pattern**: A `CameraData` class that inherits from `Camera::Private` and handles camera-specific logic.

**No standalone "Virtual Camera" class exists** - each pipeline implements its own camera data handling.

---

## Recommendation: Create a Reusable `VirtualCameraBase` Class

### Option 1: Extract to Shared Library (Recommended)

Create a reusable virtual camera base class that can be used by any pipeline:

**File Structure:**
```
src/libcamera/internal/virtual_camera/
├── virtual_camera_base.h      # Base class declaration
├── virtual_camera_base.cpp    # Core implementation
├── frame_generator.h          # Frame generation interface
├── test_pattern_generator.h   # Test pattern helpers
└── meson.build               # Build rules
```

**Base Class Design:**
```cpp
// src/libcamera/internal/virtual_camera/virtual_camera_base.h
#pragma once

#include <libcamera/base/thread.h>
#include <libcamera/base/object.h>
#include <libcamera/camera.h>
#include <libcamera/stream.h>
#include <memory>
#include <vector>

namespace libcamera {

class FrameGenerator;
class PipelineHandler;

/**
 * @brief Base class for virtual/dummy camera implementations
 * 
 * Provides common functionality for virtual cameras:
 * - Thread-based request processing
 * - Buffer allocation and management
 * - Frame generation interface
 * - IPA integration helpers
 * 
 * Usage: Inherit from this class in your pipeline's CameraData
 */
class VirtualCameraBase : public Camera::Private, public Thread {
public:
    struct Configuration {
        Size resolution;
        PixelFormat format;
        unsigned int bufferCount;
    };

    VirtualCameraBase(PipelineHandler *pipe, const Configuration &config);
    ~VirtualCameraBase() override;

    /**
     * @brief Initialize the virtual camera
     * @return 0 on success, negative error code
     */
    virtual int init();

    /**
     * @brief Configure frame generation
     * @param generator Frame generator to use
     */
    void setFrameGenerator(std::unique_ptr<FrameGenerator> generator);

    /**
     * @brief Process a single request
     * @param request Request to process
     */
    void processRequest(Request *request);

    /**
     * @brief Allocate buffers for a stream
     * @param stream Stream configuration
     * @param buffers Output buffer vector
     * @return 0 on success
     */
    int allocateBuffers(Stream *stream, 
                       std::vector<std::unique_ptr<FrameBuffer>> *buffers);

    /**
     * @brief Start camera streaming
     * @return 0 on success
     */
    virtual int start();

    /**
     * @brief Stop camera streaming
     */
    virtual void stop();

protected:
    /**
     * @brief Thread entry point
     */
    void run() override;

    /**
     * @brief Generate a frame for the given buffer
     * @param buffer Buffer to fill
     * @param sequence Frame sequence number
     * @return 0 on success
     */
    virtual int generateFrame(FrameBuffer *buffer, uint32_t sequence);

    PipelineHandler *pipe() const { return pipe_; }
    const Configuration &config() const { return config_; }

private:
    PipelineHandler *pipe_;
    Configuration config_;
    std::unique_ptr<FrameGenerator> frameGenerator_;
    std::vector<std::unique_ptr<FrameBuffer>> buffers_;
    std::queue<Request *> pendingRequests_;
    bool running_;
    uint32_t sequence_;
};

} // namespace libcamera
```

**Implementation Skeleton:**
```cpp
// src/libcamera/internal/virtual_camera/virtual_camera_base.cpp
#include "virtual_camera_base.h"
#include "frame_generator.h"
#include <libcamera/base/log.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(VirtualCamera)

VirtualCameraBase::VirtualCameraBase(PipelineHandler *pipe, 
                                     const Configuration &config)
    : Camera::Private(pipe), Thread("VirtualCamera"), 
      pipe_(pipe), config_(config), running_(false), sequence_(0)
{
    LOG(VirtualCamera, Debug) << "Creating virtual camera: " 
                              << config_.resolution.toString();
}

VirtualCameraBase::~VirtualCameraBase()
{
    stop();
}

int VirtualCameraBase::init()
{
    LOG(VirtualCamera, Info) << "Initializing virtual camera";
    return 0;
}

void VirtualCameraBase::setFrameGenerator(std::unique_ptr<FrameGenerator> gen)
{
    frameGenerator_ = std::move(gen);
}

int VirtualCameraBase::allocateBuffers(Stream *stream,
                                      std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    LOG(VirtualCamera, Debug) << "Allocating " << config_.bufferCount 
                              << " buffers";

    for (unsigned int i = 0; i < config_.bufferCount; ++i) {
        // Use memfd_create for anonymous shared memory
        int fd = memfd_create("virtual_cam_buffer", MFD_CLOEXEC);
        if (fd < 0)
            return -errno;

        size_t size = config_.resolution.width * config_.resolution.height * 3; // RGB888
        if (ftruncate(fd, size) < 0) {
            close(fd);
            return -errno;
        }

        auto buffer = std::make_unique<FrameBuffer>();
        buffer->planes().emplace_back(FrameBuffer::Plane{fd, size});
        fd.release(); // FrameBuffer takes ownership

        buffers->push_back(std::move(buffer));
    }

    return 0;
}

int VirtualCameraBase::start()
{
    LOG(VirtualCamera, Info) << "Starting virtual camera";
    running_ = true;
    start(); // Start thread
    return 0;
}

void VirtualCameraBase::stop()
{
    if (!running_)
        return;

    LOG(VirtualCamera, Debug) << "Stopping virtual camera";
    running_ = false;
    Thread::stop(); // Stop thread
}

void VirtualCameraBase::run()
{
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        // Process pending requests
        while (!pendingRequests_.empty()) {
            Request *request = pendingRequests_.front();
            processRequest(request);
            pendingRequests_.pop();
        }
    }
}

void VirtualCameraBase::processRequest(Request *request)
{
    if (!running_)
        return;

    // Get buffer from request
    const std::vector<Request::Buffer> &buffers = request->buffers();
    if (buffers.empty())
        return;

    FrameBuffer *buffer = buffers[0].buffer;
    
    // Generate frame data
    int ret = generateFrame(buffer, sequence_++);
    if (ret < 0) {
        LOG(VirtualCamera, Error) << "Failed to generate frame";
        request->complete(ret);
        return;
    }

    // Complete request
    request->complete();
}

int VirtualCameraBase::generateFrame(FrameBuffer *buffer, uint32_t sequence)
{
    if (!frameGenerator_) {
        LOG(VirtualCamera, Warning) << "No frame generator set";
        return -EINVAL;
    }

    // Map buffer memory
    const auto &plane = buffer->planes()[0];
    void *mem = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE,
                     MAP_SHARED, plane.fd.get(), 0);
    if (mem == MAP_FAILED)
        return -errno;

    // Generate frame
    int ret = frameGenerator_->generate(mem, config_.resolution, sequence);
    
    munmap(mem, plane.length);
    return ret;
}

} // namespace libcamera
```

---

### Option 2: Minimal Decoupled Class (Quick Solution)

If you want something simpler and faster:

**File:** `src/libcamera/internal/virtual_camera_stub.h`

```cpp
#pragma once

#include <libcamera/camera.h>
#include <libcamera/base/thread.h>
#include <memory>
#include <queue>

namespace libcamera {

/**
 * @brief Minimal virtual camera helper for testing
 * 
 * A tiny, standalone class that provides virtual camera functionality
 * without any dependencies on specific pipelines.
 * 
 * Usage:
 *   class MyDummyCameraData : public Camera::Private, public VirtualCameraStub {
 *   public:
 *       MyDummyCameraData(PipelineHandler *pipe) 
 *           : Camera::Private(pipe), VirtualCameraStub(pipe) {}
 *       
 *       // Override generateFrame() to customize behavior
 *       int generateFrame(FrameBuffer *buffer, uint32_t seq) override {
 *           // Your custom frame generation
 *           return 0;
 *       }
 *   };
 */
class VirtualCameraStub : public Thread {
public:
    explicit VirtualCameraStub(PipelineHandler *pipe)
        : pipe_(pipe), running_(false), sequence_(0) {}

    virtual ~VirtualCameraStub() { stop(); }

    int start() {
        running_ = true;
        Thread::start();
        return 0;
    }

    void stop() {
        if (!running_) return;
        running_ = false;
        Thread::stop();
    }

    void queueRequest(Request *request) {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingRequests_.push(request);
    }

protected:
    void run() override {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            std::lock_guard<std::mutex> lock(mutex_);
            while (!pendingRequests_.empty()) {
                Request *req = pendingRequests_.front();
                pendingRequests_.pop();
                processRequest(req);
            }
        }
    }

    virtual void processRequest(Request *request) {
        const auto &buffers = request->buffers();
        if (buffers.empty()) {
            request->complete(-EINVAL);
            return;
        }

        int ret = generateFrame(buffers[0].buffer, sequence_++);
        request->complete(ret);
    }

    virtual int generateFrame(FrameBuffer *buffer, uint32_t sequence) {
        // Default: do nothing (buffer remains unchanged)
        return 0;
    }

private:
    PipelineHandler *pipe_;
    bool running_;
    uint32_t sequence_;
    std::queue<Request *> pendingRequests_;
    std::mutex mutex_;
};

} // namespace libcamera
```

---

## Integration with dummysoftisp

### Using Option 2 (Minimal Stub)

**Before (Current):**
```cpp
// dummysoftisp/softisp.h
class DummySoftISPCameraData : public Camera::Private, public Thread {
    // 100+ lines of thread and request handling
};
```

**After (Decoupled):**
```cpp
// dummysoftisp/softisp.h
#include "libcamera/internal/virtual_camera_stub.h"

class DummySoftISPCameraData : public Camera::Private, public VirtualCameraStub {
public:
    DummySoftISPCameraData(PipelineHandlerDummysoftisp *pipe)
        : Camera::Private(pipe), VirtualCameraStub(pipe) {}

    // Much simpler! Just override what you need
    int init() {
        // Your initialization
        return 0;
    }

protected:
    int generateFrame(FrameBuffer *buffer, uint32_t sequence) override {
        // Custom frame generation for SoftISP
        // Maybe generate Bayer pattern or load test image
        return 0;
    }

private:
    // IPA and other members
    std::unique_ptr<ipa::soft::IPAProxySoft> ipa_;
};
```

---

## Benefits of Decoupling

### 1. **Reusability**
- Same virtual camera logic can be used by multiple pipelines
- No code duplication across `dummysoftisp`, `virtual`, `vimc`, etc.

### 2. **Maintainability**
- Fix bugs in one place, all pipelines benefit
- Easier to test and validate

### 3. **Flexibility**
- Pipelines can choose to use it or implement their own
- Easy to extend with new features (frame generators, patterns)

### 4. **Smaller Codebase**
- `dummysoftisp` goes from ~100 lines to ~50 lines
- Clear separation of concerns

---

## Search Results: Similar Existing Usage

### Already Using Similar Patterns:

1. **`src/libcamera/pipeline/simple/simple.cpp`**
   - Uses `SimpleCameraData` with thread-based processing
   - Similar buffer allocation logic

2. **`src/libcamera/pipeline/vimc/vimc.cpp`**
   - `VimcCameraData` with IPA integration
   - Similar structure but V4L2-specific

3. **`src/libcamera/pipeline/virtual/virtual.h`**
   - `VirtualCameraData` with `FrameGenerator` interface
   - **Closest match** but tightly coupled to virtual pipeline

### No Standalone Library Found

There is **no existing reusable virtual camera library** in libcamera. Each pipeline implements its own version, leading to code duplication.

---

## Recommendation Summary

### For Immediate Use (Today)
Use **Option 2: Minimal Stub** (`virtual_camera_stub.h`)
- Quick to implement (1 file, ~80 lines)
- No build system changes needed
- Can be used immediately in `dummysoftisp`

### For Long-term (Future)
Use **Option 1: Full Base Class** (`virtual_camera_base.h/cpp`)
- Proper abstraction with frame generators
- Build system integration
- Reusable across all pipelines
- Better for team collaboration

---

## Next Steps

1. **Create the stub file** (`src/libcamera/internal/virtual_camera_stub.h`)
2. **Update `dummysoftisp`** to use the stub
3. **Test** that everything still works
4. **Document** the pattern for future pipelines
5. **Optional**: Expand to full base class later

Would you like me to create the stub file and update `dummysoftisp` to use it?
