/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP with Simple pipeline base
 * Supports both real V4L2 cameras and virtual test cameras
 */

#pragma once

#include <memory>
#include <set>

#include <libcamera/base/mutex.h>
#include <libcamera/camera.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/pipeline_handler.h"
#include "libcamera/internal/simple_pipeline_handler.h"

#include "softisp.h"
#include "virtual_camera.h"

namespace libcamera {

/*
 * SoftISPSimplePipelineHandler - A pipeline handler that extends SimplePipelineHandler
 * to support virtual cameras with SoftISP IPA integration.
 *
 * This handler:
 * 1. First attempts to match real V4L2 cameras using the Simple pipeline infrastructure
 * 2. Falls back to creating a virtual camera if no real cameras are found
 * 3. Integrates SoftISP IPA for both real and virtual cameras
 */
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

    int queueRequestDevice(Camera *camera, Request *request) override;

private:
    bool isVirtualCamera(Camera *camera);
    bool createVirtualCamera();
    int exportVirtualBuffers(Stream *stream,
                            std::vector<std::unique_ptr<FrameBuffer>> *buffers);
    void processVirtualRequest(Request *request);
    bool initVirtualCameraIPA(SoftISPCameraData *cameraData);

    std::set<Camera *> virtualCameras_;
    Mutex virtualCamerasMutex_;
};

} // namespace libcamera
