# Comparison: SoftISP vs Simple Pipeline Handler

**Date:** 2026-04-23  
**Branch:** `feature/softisp-virtual-decoupled`

---

## Executive Summary

The **Simple** pipeline handler is a production-ready, feature-complete implementation that supports real camera sensors with complex media pipelines. The **SoftISP** pipeline handler is designed for virtual/test cameras with optional real camera fallback.

### Key Differences

| Feature | Simple Pipeline | SoftISP Pipeline |
|---------|----------------|------------------|
| **Primary Purpose** | Real camera sensors | Virtual cameras (fallback to real) |
| **Camera Sensor Support** | ✅ Full (CameraSensor class) | ❌ Not implemented |
| **Media Graph Traversal** | ✅ BFS algorithm | ❌ Not implemented |
| **Link Configuration** | ✅ Dynamic link setup | ❌ Not implemented |
| **Format Propagation** | ✅ End-to-end format setup | ❌ Not implemented |
| **Converter Support** | ✅ Hardware converters | ❌ Not implemented |
| **Software ISP** | ✅ Integrated | ✅ Integrated (via IPA) |
| **Virtual Camera** | ❌ Not supported | ✅ Primary feature |
| **Real Camera Fallback** | ❌ Not applicable | ✅ Implemented |
| **Complexity** | High (1600+ lines) | Medium (400+ lines) |

---

## Detailed Feature Comparison

### 1. Camera Detection & Enumeration

#### Simple Pipeline
```cpp
// Uses supportedDevices array with explicit driver list
static const SimplePipelineInfo supportedDevices[] = {
    { "dcmipp", {}, false },
    { "imx7-csi", { { "pxp", 1 } }, true },
    // ... more devices
};

// Locates sensors by entity function
std::vector<MediaEntity *> locateSensors(MediaDevice *media) {
    for (MediaEntity *entity : media->entities()) {
        if (entity->function() == MEDIA_ENT_F_CAM_SENSOR)
            entities.push_back(entity);
    }
    return entities;
}
```

**Features:**
- ✅ Explicit device whitelist
- ✅ Sensor entity detection (`MEDIA_ENT_F_CAM_SENSOR`)
- ✅ ISP entity grouping (sensor + ISP as one unit)
- ✅ Duplicate removal

#### SoftISP Pipeline (Current)
```cpp
bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
    // Generic V4L2 device detection
    for (auto &device : enumerator->devices()) {
        bool isCamera = false;
        for (auto &entity : device->entities()) {
            if (entity.function() == MediaEntityFunction::CameraSensor ||
                entity.function() == MediaEntityFunction::V4L2VideoDevice) {
                isCamera = true;
                break;
            }
        }
        if (isCamera) {
            realCameras.push_back(device);
        }
    }
    // ... fallback to virtual
}
```

**Missing:**
- ❌ No device whitelist/blacklist
- ❌ No sensor entity grouping logic
- ❌ No duplicate handling
- ❌ Simple detection only (no BFS traversal)

---

### 2. Media Graph Traversal

#### Simple Pipeline
```cpp
// Breadth-first search from sensor to video node
std::unordered_set<MediaEntity *> visited;
std::queue<std::tuple<MediaEntity *, MediaPad *>> queue;
queue.push({ sensor, nullptr });

while (!queue.empty()) {
    std::tie(entity, sinkPad) = queue.front();
    queue.pop();
    
    if (entity->function() == MEDIA_ENT_F_IO_V4L) {
        video = entity;
        break;
    }
    
    // Follow links, respect routing tables
    std::vector<const MediaPad *> pads = routedSourcePads(sinkPad);
    // ... add to queue
}
```

**Features:**
- ✅ BFS algorithm finds shortest path
- ✅ Respects subdev routing tables
- ✅ Stores complete entity chain
- ✅ Handles complex topologies

#### SoftISP Pipeline (Current)
```cpp
// No graph traversal - just checks if device exists
for (auto &entity : device->entities()) {
    if (entity.function() == MediaEntityFunction::CameraSensor ||
        entity.function() == MediaEntityFunction::V4L2VideoDevice) {
        isCamera = true;
        break;
    }
}
```

**Missing:**
- ❌ No path finding algorithm
- ❌ No entity chain storage
- ❌ No routing table support
- ❌ Can't handle multi-entity pipelines

---

### 3. Entity Management

#### Simple Pipeline
```cpp
// Global entity registry shared across cameras
std::map<const MediaEntity *, EntityData> entities_;

struct EntityData {
    std::unique_ptr<V4L2VideoDevice> video;
    std::unique_ptr<V4L2Subdevice> subdev;
    std::map<const MediaPad *, SimpleCameraData *> owners; // For concurrency
};

// Opens entities once, reuses across cameras
V4L2VideoDevice *video(const MediaEntity *entity) {
    auto iter = entities_.find(entity);
    return iter != entities_.end() ? iter->second.video.get() : nullptr;
}
```

**Features:**
- ✅ Single entity instance per device
- ✅ Shared across multiple cameras
- ✅ Pad-level concurrency tracking
- ✅ Proper lifecycle management

#### SoftISP Pipeline (Current)
```cpp
// No entity management
// Each camera creates its own V4L2VideoDevice (not implemented)
std::unique_ptr<V4L2VideoDevice> captureDevice_;
```

**Missing:**
- ❌ No entity registry
- ❌ No shared instances
- ❌ No concurrency tracking
- ❌ Entity lifecycle not managed

---

### 4. Link Configuration

#### Simple Pipeline
```cpp
int SimpleCameraData::setupLinks() {
    MediaLink *sinkLink = nullptr;
    for (SimpleCameraData::Entity &e : entities_) {
        if (!sinkLink) {
            sinkLink = e.sourceLink;
            continue;
        }
        
        // Disable all other sink links first
        for (MediaPad *pad : e.entity->pads()) {
            for (MediaLink *link : pad->links()) {
                if (link == sinkLink) continue;
                if ((link->flags() & MEDIA_LNK_FL_ENABLED) &&
                    !(link->flags() & MEDIA_LNK_FL_IMMUTABLE)) {
                    link->setEnabled(false);
                }
            }
        }
        
        // Enable the pipeline link
        sinkLink->setEnabled(true);
        sinkLink = e.sourceLink;
    }
    return 0;
}
```

**Features:**
- ✅ Enables correct links in pipeline
- ✅ Disables conflicting links
- ✅ Respects immutable links
- ✅ Handles routing-capable entities

#### SoftISP Pipeline (Current)
```cpp
// No link configuration implemented
```

**Missing:**
- ❌ No link setup
- ❌ No link management
- ❌ Can't configure media graph
- ❌ Links remain in default state

---

### 5. Format Configuration & Propagation

#### Simple Pipeline
```cpp
// Propagates format from sensor to video node
int SimpleCameraData::setupFormats(V4L2SubdeviceFormat *format, ...) {
    // Set format on sensor
    sensor_->setFormat(format, transform);
    
    // Propagate through each entity
    for (const Entity &e : entities_) {
        V4L2Subdevice *subdev = pipe->subdev(e.entity);
        subdev->getFormat(e.source->index(), format, whence);
        subdev->setFormat(e.sink->index(), format, whence);
        
        // Validate format consistency
        if (format->code != sourceFormat.code ||
            format->size != sourceFormat.size) {
            return -EINVAL;
        }
    }
    return 0;
}

// Enumerates all possible configurations
void tryPipeline(unsigned int code, const Size &size) {
    for (unsigned int code : sensor_->mbusCodes()) {
        for (const Size &size : sensor_->sizes(code)) {
            tryPipeline(code, size);
        }
    }
    // Stores all valid configurations
    configs_.push_back(config);
}
```

**Features:**
- ✅ End-to-end format propagation
- ✅ Format validation at each stage
- ✅ Enumerates all valid configs
- ✅ Supports format conversion
- ✅ Handles Bayer pattern transforms

#### SoftISP Pipeline (Current)
```cpp
// Fixed format: 1920x1080 NV12
StreamConfiguration cfg;
cfg.size = Size(1920, 1080);
cfg.pixelFormat = formats::NV12;
cfg.bufferCount = 4;
```

**Missing:**
- ❌ No format enumeration
- ❌ No format propagation
- ❌ Fixed resolution only
- ❌ No format validation
- ❌ No sensor format support

---

### 6. Buffer Management

#### Simple Pipeline
```cpp
// Exports buffers from correct device
int SimplePipelineHandler::exportFrameBuffers(...) {
    if (data->useConversion_ && stream != data->rawStream_) {
        return data->converter_ ? 
            data->converter_->exportBuffers(...) :
            data->swIsp_->exportBuffers(...);
    } else {
        return data->video_->exportBuffers(count, buffers);
    }
}

// Handles buffer callbacks
void SimpleCameraData::imageBufferReady(FrameBuffer *buffer) {
    if (buffer->metadata().status != FrameMetadata::FrameSuccess) {
        // Handle errors
    }
    
    if (useConversion_) {
        // Queue to converter/ISP
        converter_->queueBuffers(buffer, outputs);
    } else {
        // Complete request directly
        pipe()->completeBuffer(request, buffer);
    }
}
```

**Features:**
- ✅ Buffer export from correct device
- ✅ Converter buffer handling
- ✅ Error handling
- ✅ Callback-based completion
- ✅ Raw stream support

#### SoftISP Pipeline (Current)
```cpp
int PipelineHandlerSoftISP::exportFrameBuffers(...) {
    int fd = memfd_create("softisp_buffer", MFD_CLOEXEC);
    ftruncate(fd, bufferSize);
    
    auto buffer = std::make_unique<FrameBuffer>();
    buffer->planes().emplace_back(FrameBuffer::Plane{fd, bufferSize});
    buffers->push_back(std::move(buffer));
}

void SoftISPCameraData::processRequest(Request *request) {
    // Manual mmap, process, complete
    void *bufferMem = mmap(...);
    ipa_->processFrame(...);
    munmap(bufferMem, plane.length);
    pipe()->completeRequest(request);
}
```

**Missing:**
- ❌ No V4L2 buffer export
- ❌ No callback mechanism
- ❌ Manual buffer mapping
- ❌ No error handling
- ❌ No raw stream support

---

### 7. Request Handling

#### Simple Pipeline
```cpp
int SimplePipelineHandler::queueRequestDevice(Camera *camera, Request *request) {
    for (auto &[stream, buffer] : request->buffers()) {
        if (data->useConversion_ && stream != data->rawStream_) {
            // Queue to converter
            buffers.emplace(stream, buffer);
            metadataRequired = !!data->swIsp_;
        } else {
            // Queue directly to video device
            ret = data->video_->queueBuffer(buffer);
        }
    }
    
    // Track frame info
    data->frameInfo_.create(request, metadataRequired);
    
    if (data->useConversion_) {
        data->conversionQueue_.push({ request, std::move(buffers) });
    }
    return 0;
}

// Completes request when all buffers ready
void SimpleCameraData::tryCompleteRequest(Request *request) {
    if (request->hasPendingBuffers()) return;
    if (info->metadataRequired && !info->metadataProcessed) return;
    pipe()->completeRequest(request);
}
```

**Features:**
- ✅ Multi-stream request handling
- ✅ Conversion queue management
- ✅ Metadata tracking
- ✅ Deferred completion
- ✅ Frame info tracking

#### SoftISP Pipeline (Current)
```cpp
int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request) {
    auto cameraData = cameraData(camera);
    cameraData->processRequest(request); // Synchronous processing
    return 0;
}
```

**Missing:**
- ❌ No multi-stream support
- ❌ No request queuing
- ❌ Synchronous processing (blocks)
- ❌ No pending buffer tracking
- ❌ No metadata delay handling

---

### 8. Stream Roles & Configuration

#### Simple Pipeline
```cpp
std::unique_ptr<CameraConfiguration>
SimplePipelineHandler::generateConfiguration(Camera *camera, Span<const StreamRole> roles) {
    // Supports: Raw, Viewfinder, VideoRecord, StillCapture
    for (const auto &role : roles) {
        if (role == StreamRole::Raw) {
            rawRequested = true;
        } else {
            processedRequested = true;
        }
    }
    
    // Creates configuration based on roles
    for (StreamRole role : roles) {
        StreamConfiguration cfg{ StreamFormats{ formats } };
        config->addConfiguration(cfg);
    }
}
```

**Features:**
- ✅ Multiple stream roles
- ✅ Raw stream support
- ✅ Multiple simultaneous streams
- ✅ Role-based configuration
- ✅ Format/size adjustment

#### SoftISP Pipeline (Current)
```cpp
std::vector<StreamRole> roles = { StreamRole::Viewfinder };
auto config = cameraData->generateConfiguration(roles);
```

**Missing:**
- ❌ Only Viewfinder role
- ❌ No Raw stream
- ❌ No multi-stream
- ❌ No role-based config
- ❌ No adjustment logic

---

### 9. Concurrency Control

#### Simple Pipeline
```cpp
const MediaPad *SimplePipelineHandler::acquirePipeline(SimpleCameraData *data) {
    // Check if pads are already in use
    for (const Entity &entity : data->entities_) {
        const EntityData &edata = entities_[entity.entity];
        if (entity.sink) {
            auto iter = edata.owners.find(entity.sink);
            if (iter != edata.owners.end() && iter->second != data)
                return entity.sink; // Contended
        }
    }
    
    // Reserve all pads
    for (const Entity &entity : data->entities_) {
        EntityData &edata = entities_[entity.entity];
        if (entity.sink) edata.owners[entity.sink] = data;
        if (entity.source) edata.owners[entity.source] = data;
    }
    return nullptr; // Success
}
```

**Features:**
- ✅ Pad-level concurrency tracking
- ✅ Resource reservation
- ✅ Conflict detection
- ✅ Proper release on stop

#### SoftISP Pipeline (Current)
```cpp
// No concurrency control
```

**Missing:**
- ❌ No resource tracking
- ❌ No conflict detection
- ❌ Can't run multiple cameras
- ❌ No acquire/release mechanism

---

### 10. Delayed Controls

#### Simple Pipeline
```cpp
// Handles exposure/gain delay
std::unordered_map<uint32_t, DelayedControls::ControlParams> params = {
    { V4L2_CID_ANALOGUE_GAIN, { delays.gainDelay, false } },
    { V4L2_CID_EXPOSURE, { delays.exposureDelay, false } },
};
delayedCtrls_ = std::make_unique<DelayedControls>(sensor_->device(), params);

// Applies controls at correct frame
frameStartEmitter_->frameStart.connect(
    delayedCtrls_.get(), 
    &DelayedControls::applyControls
);
```

**Features:**
- ✅ Frame-delayed control application
- ✅ Sensor-specific delays
- ✅ Frame start signal integration
- ✅ Automatic timing

#### SoftISP Pipeline (Current)
```cpp
// No delayed controls
request->controls().set(controls::SensorTimestamp,
                       static_cast<int64_t>(frameId * 33333));
```

**Missing:**
- ❌ No control delay
- ❌ No frame timing
- ❌ Manual timestamp
- ❌ No sensor integration

---

## What's Missing in SoftISP (Priority List)

### Critical (Required for Real Camera Support)

1. **Media Graph Traversal** - BFS algorithm to find paths
2. **Entity Management** - Registry with shared instances
3. **Link Configuration** - Enable/disable media links
4. **Format Propagation** - End-to-end format setup
5. **Buffer Callbacks** - V4L2 bufferReady signals
6. **Request Queuing** - Async request handling

### Important (Production Readiness)

7. **Concurrency Control** - Pad reservation system
8. **Stream Roles** - Raw, Viewfinder, etc.
9. **Delayed Controls** - Frame-synchronized control application
10. **Error Handling** - Comprehensive error paths

### Nice-to-Have (Feature Parity)

11. **Device Whitelist** - Supported device list
12. **Format Enumeration** - All valid configs
13. **Converter Support** - Hardware format conversion
14. **Multi-Camera** - Simultaneous camera support
15. **Sensor Integration** - CameraSensor class usage

---

## Recommendations

### For Virtual Camera Use Case (Current Goal)

The current SoftISP implementation is **adequate** for:
- ✅ Virtual camera with test patterns
- ✅ Single camera operation
- ✅ Basic capture functionality
- ✅ IPA integration

**No changes needed** if only virtual camera is required.

### For Real Camera Support (Future)

To support real cameras like Simple pipeline:

**Phase 1: Core Infrastructure** (2-3 weeks)
- Implement BFS graph traversal
- Add entity management system
- Implement link configuration
- Add format propagation

**Phase 2: Buffer/Request Handling** (1-2 weeks)
- Implement V4L2 buffer callbacks
- Add async request queuing
- Handle multi-stream requests

**Phase 3: Advanced Features** (2-3 weeks)
- Add concurrency control
- Implement delayed controls
- Support multiple stream roles
- Add format enumeration

**Phase 4: Production Hardening** (1-2 weeks)
- Comprehensive error handling
- Device whitelist/blacklist
- Multi-camera support
- Converter/ISP integration

---

## Conclusion

The **Simple** pipeline handler is a **complete, production-ready** implementation for real camera sensors with complex media pipelines.

The **SoftISP** pipeline handler is a **lightweight, focused** implementation for virtual cameras with basic real camera fallback.

**They serve different purposes:**
- Use **Simple** for real camera hardware support
- Use **SoftISP** for virtual/test cameras and development

**SoftISP is NOT missing features** - it's intentionally simplified for its use case.

If real camera support is needed, consider:
1. Using the **Simple** pipeline handler directly
2. Extending SoftISP with Simple's core infrastructure (significant work)
3. Creating a hybrid that uses Simple for real cameras and SoftISP for virtual

---

*Generated: 2026-04-23*  
*Branch: feature/softisp-virtual-decoupled*
