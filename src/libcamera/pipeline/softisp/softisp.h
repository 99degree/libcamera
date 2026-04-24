/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (virtual and real cameras)
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <libcamera/base/object.h>
#include <libcamera/base/thread.h>
#include <libcamera/base/mutex.h>
#include <libcamera/camera.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/pipeline_handler.h"
#include "libcamera/internal/dma_buf_allocator.h"
#include <libcamera/ipa/softisp_ipa_proxy.h>

namespace libcamera {

// Forward declaration for SoftISPCameraData (defined in softisp_camera.h)
class SoftISPCameraData;

class SoftISPConfiguration : public libcamera::CameraConfiguration {
public:
    SoftISPConfiguration();
    Status validate() override;
};

/* Forward declarations */
class PipelineHandlerSoftISP;
class VirtualCamera;

/*
 * SoftISPCameraData - Camera data structure for SoftISP pipeline.
 * Supports both real V4L2 cameras and virtual test cameras.
 */

/*
 * Pipeline handler for SoftISP with both real and virtual cameras.
 * Prioritizes real V4L2 cameras, falls back to virtual camera if none found.
 */
class PipelineHandlerSoftISP : public PipelineHandler {
public:
    static bool created_;
	static bool s_virtualCameraRegistered;
	bool resetCreated_ = false;
	std::shared_ptr<Camera> virtualCamera_; // Keep reference to prevent handler destruction

    PipelineHandlerSoftISP(CameraManager *manager);
    ~PipelineHandlerSoftISP();

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
    SoftISPCameraData *cameraData(Camera *camera) {
(void)camera;
        return nullptr; // static_cast<SoftISPCameraData *>(camera->_d()); // TODO: Fix cast
    }

    bool isV4LCamera(std::shared_ptr<MediaDevice> media);
    bool createRealCamera(std::shared_ptr<MediaDevice> media);
    bool createVirtualCamera();
};

} // namespace libcamera
