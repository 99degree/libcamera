/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP
 */

#include "softisp.h"
#include "softisp_camera.h"

#include <libcamera/base/log.h>

namespace libcamera {

// SoftISPConfiguration implementation
SoftISPConfiguration::SoftISPConfiguration()
{
}


CameraConfiguration::Status SoftISPConfiguration::validate()
{
    if (empty())
        return Invalid;

    Status status = Valid;
    for (auto it = begin(); it != end(); ++it) {
        StreamConfiguration &cfg = *it;

        if (cfg.size.width == 0 || cfg.size.height == 0)
            return Invalid;

        if (cfg.pixelFormat == 0)
            return Invalid;
    }

    return status;
}



// Log categories
LOG_DEFINE_CATEGORY(SoftISPPipeline)
LOG_DEFINE_CATEGORY(SoftISPCameraData)

// Static member
bool PipelineHandlerSoftISP::created_ = false;

// Constructor
PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager)
    : PipelineHandler(manager)
{
    LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler created";
}

// Destructor
PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
    if (resetCreated_)
        created_ = false;
}

// match()
bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
    (void)enumerator;
    
    if (!created_) {
        std::unique_ptr<SoftISPCameraData> data =
            std::make_unique<SoftISPCameraData>(this);

        int ret = data->init();
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize camera data";
            return false;
        }

        // Camera registration handled by CameraManager
        LOG(SoftISPPipeline, Info) << "Virtual camera created successfully";
        created_ = true;
        resetCreated_ = true;
    }

    return true;
}

// generateConfiguration()
std::unique_ptr<CameraConfiguration> PipelineHandlerSoftISP::generateConfiguration(
    Camera *camera, Span<const StreamRole> roles)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return nullptr;
    }

    LOG(SoftISPPipeline, Info) << "PipelineHandlerSoftISP::generateConfiguration called";
    return data->generateConfiguration(roles);
}

// configure()
int PipelineHandlerSoftISP::configure(Camera *camera, CameraConfiguration *config)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }

    return data->configure(config);
}

// exportFrameBuffers()
int PipelineHandlerSoftISP::exportFrameBuffers(Camera *camera, Stream *stream,
                                               std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }

    return data->exportFrameBuffers(stream, buffers);
}

// start()
int PipelineHandlerSoftISP::start(Camera *camera, const ControlList *controls)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }

    return data->start(controls);
}

// stopDevice()
void PipelineHandlerSoftISP::stopDevice(Camera *camera)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return;
    }

    data->stop();
}

// queueRequestDevice()
int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }

    return data->queueRequest(request);
}

// cameraData() - inline in header, but we can define out-of-class if needed
// SoftISPCameraData *PipelineHandlerSoftISP::cameraData(Camera *camera) { ... }

// Register the pipeline handler
REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "SoftISP")

} // namespace libcamera
