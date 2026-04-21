/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (real cameras)
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <libcamera/base/object.h>
#include <libcamera/camera.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/pipeline_handler.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/internal/v4l2_device.h"

#include <libcamera/ipa/soft_ipa_interface.h>
#include <libcamera/ipa/soft_ipa_proxy.h>

namespace libcamera {

/* Forward declarations */
class V4L2VideoDevice;
class PipelineHandlerSoftISP;

/*
 * SoftISPCameraData - Camera data structure for SoftISP pipeline.
 */
class SoftISPCameraData : public Camera::Private {
public:
	SoftISPCameraData(PipelineHandlerSoftISP *pipe);
	~SoftISPCameraData();

	int init();
	int loadIPA();

	std::unique_ptr<ipa::soft::IPAProxySoft> ipa_;
	
	/* V4L2 device members */
	std::unique_ptr<V4L2VideoDevice> captureDevice_;
	std::vector<std::unique_ptr<FrameBuffer>> buffers_;
	bool streaming_ = false;
};

/*
 * Pipeline handler for SoftISP with real cameras.
 */
class PipelineHandlerSoftISP : public PipelineHandler {
public:
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
		return static_cast<SoftISPCameraData *>(camera->_d());
	}
};

} /* namespace libcamera */
