/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024 George Chan <gchan9527@gmail.com>
 *
 * Pipeline handler for SoftISP
 */

#include "softisp.h"
#include "softisp_camera.h"

#include "softisp_camera.h"
#include "virtual_camera.h"

#include <libcamera/base/log.h>
#include <libcamera/camera_manager.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPPipeline)

// ============================================================================
// Static members
// ============================================================================
bool PipelineHandlerSoftISP::created_ = false;
bool PipelineHandlerSoftISP::s_virtualCameraRegistered = false;

// ============================================================================
// Constructor
// ============================================================================
PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager)
    : PipelineHandler(manager)
{
    LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler created";
}

// ============================================================================
// Destructor
// ============================================================================
PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
    LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler destroyed";
    
    // Reset flags if we created the virtual camera
    if (resetCreated_) {
        created_ = false;
        s_virtualCameraRegistered = false;
        resetCreated_ = false;
    }
}

// ============================================================================
// match() - Register virtual camera on first call only
// ============================================================================
bool PipelineHandlerSoftISP::match([[maybe_unused]] DeviceEnumerator *enumerator)
{
    // First call: Register the virtual camera
    if (!s_virtualCameraRegistered) {
        LOG(SoftISPPipeline, Info) << "Registering virtual camera (first match call)";
        
        // Create camera data
        std::unique_ptr<SoftISPCameraData> data = std::make_unique<SoftISPCameraData>(this);
        int ret = data->init();
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize camera data";
            return false;
        }
        
        // Store in map
        virtualCameraData_ = std::move(data);
        
        // Mark as registered
        s_virtualCameraRegistered = true;
        created_ = true;
        resetCreated_ = true;
        
        LOG(SoftISPPipeline, Info) << "Virtual camera registered successfully";
        return true;  // Camera registered, keep handler alive
    }
    
    // Subsequent calls: Return false to prevent re-registration
    LOG(SoftISPPipeline, Debug) << "Virtual camera already registered";
    return false;
}

// ============================================================================
// generateConfiguration()
// ============================================================================
std::unique_ptr<CameraConfiguration> PipelineHandlerSoftISP::generateConfiguration(
    Camera *camera, Span<const StreamRole> roles)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return nullptr;
    }
    
    return data->generateConfiguration(roles);
}

// ============================================================================
// configure()
// ============================================================================
int PipelineHandlerSoftISP::configure(Camera *camera, CameraConfiguration *config)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }
    
    return data->configure(config);
}

// ============================================================================
// exportFrameBuffers()
// ============================================================================
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

// ============================================================================
// start()
// ============================================================================
int PipelineHandlerSoftISP::start(Camera *camera, const ControlList *controls)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }
    
    return data->start(controls);
}

// ============================================================================
// stopDevice()
// ============================================================================
void PipelineHandlerSoftISP::stopDevice(Camera *camera)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return;
    }
    
    data->stop();
}

// ============================================================================
// queueRequestDevice()
// ============================================================================
int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }
    
    return data->queueRequest(request);
}

// ============================================================================
// ============================================================================

// ============================================================================
// REGISTER_PIPELINE_HANDLER macro
// ============================================================================
REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "SoftISP")

} // namespace libcamera
