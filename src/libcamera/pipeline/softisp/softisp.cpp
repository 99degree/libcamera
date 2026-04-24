/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP
 */

#include "softisp.h"

#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <queue>
#include <sys/mman.h>
#include <unistd.h>

#include <libcamera/base/log.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"

#include "virtual_camera.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPPipeline)
LOG_DEFINE_CATEGORY(SoftISPCameraData)

// Forward declaration
class SoftISPCameraData;

static std::map<uint32_t, int> g_bufferFdMap;

/* ============================================================================
 * SoftISPConfiguration
 * ============================================================================
 */

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

/* ============================================================================
 * PipelineHandlerSoftISP
 * ============================================================================
 */

bool PipelineHandlerSoftISP::created_ = false;

PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager)
    : PipelineHandler(manager)
{
    LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler created";
}

PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
    if (resetCreated_)
        created_ = false;
}

bool PipelineHandlerSoftISP::match([[maybe_unused]] MediaDevice *media,
                                   std::vector<std::shared_ptr<Camera>> *cameras)
{
    if (!created_) {
        std::shared_ptr<Camera> camera;
        std::unique_ptr<SoftISPCameraData> data =
            std::make_unique<SoftISPCameraData>(this);

        int ret = data->init();
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize camera data";
            return false;
        }

        camera = std::make_shared<Camera>(std::move(data));
        cameras->push_back(camera);
        LOG(SoftISPPipeline, Info) << "Virtual camera registered successfully";
        created_ = true;
        resetCreated_ = true;
    }

    return true;
}

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

int PipelineHandlerSoftISP::configure(Camera *camera, CameraConfiguration *config)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }

    return data->configure(config);
}

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

int PipelineHandlerSoftISP::start(Camera *camera, const ControlList *controls)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }

    return data->start(controls);
}

int PipelineHandlerSoftISP::stopDevice(Camera *camera)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }

    data->stop();
    return 0;
}

int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }

    return data->queueRequest(request);
}

SoftISPCameraData *PipelineHandlerSoftISP::cameraData(Camera *camera) const
{
    return static_cast<SoftISPCameraData *>(camera->privateData());
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "SoftISP")

} // namespace libcamera
