/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024 George Chan <gchan9527@gmail.com>
 *
 * SoftISPCameraData - Camera Object implementation
 */

#include "softisp_camera.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libcamera/base/log.h>
#include <libcamera/camera.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/stream.h>

#include "softisp.h"
#include "virtual_camera.h"

namespace libcamera {

LOG_DECLARE_CATEGORY(SoftISPCameraData)

SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
    : Camera::Private(pipe),
      Thread("SoftISPCamera"),
      virtualCamera_(std::make_unique<VirtualCamera>())
{
    LOG(SoftISPCameraData, Info) << "SoftISPCameraData created";
}

SoftISPCameraData::~SoftISPCameraData()
{
    LOG(SoftISPCameraData, Info) << "SoftISPCameraData destroyed";
    Thread::exit(0);
    wait();
}

int SoftISPCameraData::init()
{
    LOG(SoftISPCameraData, Info) << "Initializing SoftISPCameraData";
    
    // Initialize VirtualCamera with default resolution
    int ret = virtualCamera_->init(1920, 1080);
    if (ret < 0) {
        LOG(SoftISPCameraData, Error) << "Failed to initialize VirtualCamera";
        return ret;
    }

    LOG(SoftISPCameraData, Info) << "SoftISPCameraData initialized";
    return 0;
}

std::unique_ptr<CameraConfiguration> SoftISPCameraData::generateConfiguration(
    Span<const StreamRole> roles)
{
    LOG(SoftISPCameraData, Info) << "SoftISPCameraData::generateConfiguration called";
    
    if (!virtualCamera_) {
        LOG(SoftISPCameraData, Error) << "VirtualCamera not initialized";
        return nullptr;
    }

    auto config = std::make_unique<SoftISPConfiguration>();
    if (roles.empty()) {
        return config;
    }

    for (const auto& role : roles) {
        switch (role) {
            case StreamRole::StillCapture:
            case StreamRole::VideoRecording:
            case StreamRole::Viewfinder:
                break;
            case StreamRole::Raw:
            default:
                LOG(SoftISPCameraData, Error) << "Unsupported stream role: " << role;
                return nullptr;
        }

        unsigned int width = virtualCamera_->width();
        unsigned int height = virtualCamera_->height();
        unsigned int bufferCount = virtualCamera_->bufferCount();

        std::map<PixelFormat, std::vector<SizeRange>> streamFormats;
        PixelFormat pixelFormat = formats::SBGGR10;
        streamFormats[pixelFormat] = { SizeRange(Size(width, height), Size(width, height)) };

        StreamFormats formats(streamFormats);
        auto cfg = StreamConfiguration(formats);
        cfg.pixelFormat = pixelFormat;
        cfg.size = Size(width, height);
        cfg.bufferCount = bufferCount;
        cfg.colorSpace = ColorSpace::Rec709;

        config->addConfiguration(cfg);
        LOG(SoftISPCameraData, Info) << "Added stream: " << width << "x" << height;
    }

    if (config->validate() == CameraConfiguration::Invalid) {
        LOG(SoftISPCameraData, Error) << "Invalid configuration";
        return nullptr;
    }

    return config;
}

void SoftISPCameraData::run()
{
    // Thread loop (placeholder)
    while (true) {
        break;
    }
}

// TODO: Implement other methods (configure, exportFrameBuffers, start, stop, queueRequest)

// Configure the camera
int SoftISPCameraData::configure(CameraConfiguration *config)
{
    LOG(SoftISPCameraData, Info) << "SoftISPCameraData::configure called with " << config->size() << " streams";
    
    if (!virtualCamera_) {
        LOG(SoftISPCameraData, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }
    
    // Store the configuration for later use
    // In a real implementation, we would configure the hardware here
    // For now, we just log that we received the configuration
    
    return 0;
}

// Export frame buffers
int SoftISPCameraData::exportFrameBuffers([[maybe_unused]] Stream *stream, [[maybe_unused]] std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    LOG(SoftISPCameraData, Info) << "SoftISPCameraData::exportFrameBuffers called";
    
    if (!virtualCamera_) {
        LOG(SoftISPCameraData, Error) << "Invalid parameters";
        return -EINVAL;
    }
    
    // Frame buffer export is typically handled by the pipeline handler
    // or the hardware device. For a virtual camera, we might create
    // synthetic buffers here.
    // For now, return not supported as this is complex to implement
    return -ENOTSUP;
}

// Start the camera
int SoftISPCameraData::start([[maybe_unused]] const ControlList *controls)
{
    LOG(SoftISPCameraData, Info) << "SoftISPCameraData::start called";
    
    if (!virtualCamera_) {
        LOG(SoftISPCameraData, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }
    
    // Start the virtual camera thread
    // This would typically start the frame generation loop
    // start(); // Start the Thread base class
    
    return 0;
}

// Stop the camera
void SoftISPCameraData::stop()
{
    LOG(SoftISPCameraData, Info) << "SoftISPCameraData::stop called";
    
    if (virtualCamera_) {
        // Stop the virtual camera
        // This would typically stop the frame generation loop
        // exit(0);
        // wait();
    }
}

// Queue a request for processing
int SoftISPCameraData::queueRequest(Request *request)
{
    LOG(SoftISPCameraData, Info) << "SoftISPCameraData::queueRequest called";
    
    if (!virtualCamera_ || !request) {
        LOG(SoftISPCameraData, Error) << "Invalid parameters";
        return -EINVAL;
    }
    
    // In a real implementation, we would:
    // 1. Get the buffer from the request
    // 2. Process it through the ONNX model
    // 3. Queue the processed frame
    // 4. Complete the request
    
    // For now, we just log the request
    return 0;
}



// Load IPA module
int SoftISPCameraData::loadIPA()
{
    LOG(SoftISPCameraData, Info) << "Loading IPA module";
    // TODO: Implement IPA loading logic
    // This was in the original but may not be needed for virtual camera
    return 0;
}

// Get buffer from ID
FrameBuffer *SoftISPCameraData::getBufferFromId(uint32_t bufferId)
{
    LOG(SoftISPCameraData, Debug) << "Getting buffer from ID: " << bufferId;
    // TODO: Implement buffer retrieval logic
    // This maps buffer IDs to actual FrameBuffer pointers
    return nullptr;
}

// Store buffer
void SoftISPCameraData::storeBuffer(uint32_t bufferId, [[maybe_unused]] FrameBuffer *buffer)
{
    LOG(SoftISPCameraData, Debug) << "Storing buffer ID: " << bufferId;
    // TODO: Implement buffer storage logic
    // This stores the buffer mapping for later retrieval
}

// Process request (CRITICAL for frame capture)
void SoftISPCameraData::processRequest([[maybe_unused]] Request *request)
{
    LOG(SoftISPCameraData, Info) << "Processing request";
    
    if (!virtualCamera_) {
        LOG(SoftISPCameraData, Error) << "VirtualCamera not initialized";
        return;
    }
    
    // In the original implementation, this would:
    // 1. Get the buffer from the request
    // 2. Process it through the ONNX model (algo.onnx + applier.onnx)
    // 3. Generate the output frame
    // 4. Queue the processed frame back to the virtual camera
    // 5. Complete the request
    
    // For now, we just log that we received a request
    // TODO: Implement full ONNX processing pipeline
    
    // Example (from original pattern):
    // FrameBuffer *buffer = getBufferFromId(bufferId);
    // if (buffer) {
    //     // Process through ONNX
    //     // Queue to virtual camera
    //     virtualCamera_->queueBuffer(buffer);
    // }
    
    LOG(SoftISPCameraData, Info) << "Request processing complete (placeholder)";
}


} // namespace libcamera
