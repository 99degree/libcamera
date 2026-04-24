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

} // namespace libcamera
