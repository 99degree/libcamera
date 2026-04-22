/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (real cameras)
 */
#pragma once

#include <memory>

#include <libcamera/base/thread.h>
#include <libcamera/base/utils.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/internal/pipeline_handler.h"

namespace libcamera {

class SoftISPCameraData : public Camera::Private
{
public:
	SoftISPCameraData(PipelineHandlerSoftISP *pipe);
	~SoftISPCameraData() override;

	int init();
	int loadIPA();

	void processRequest(Request *request);

	std::unique_ptr<ipa::soft::IPAProxySoft> ipa_;
};

class PipelineHandlerSoftISP : public PipelineHandler
{
public:
	PipelineHandlerSoftISP(CameraManager *manager);
	~PipelineHandlerSoftISP() override;

	bool match(DeviceEnumerator *enumerator) override;
	std::unique_ptr<CameraConfiguration> generateConfiguration(Camera *camera,
								   Span<const StreamRole> roles) override;
	int configure(Camera *camera, CameraConfiguration *config) override;
	int exportFrameBuffers(Camera *camera, Stream *stream,
			       std::vector<std::unique_ptr<FrameBuffer>> *buffers) override;
	int start(Camera *camera, const ControlList *controls) override;
	void stopDevice(Camera *camera) override;
	int queueRequestDevice(Camera *camera, Request *request) override;

protected:
	SoftISPCameraData *cameraData(const Camera *camera) const
	{
		return static_cast<SoftISPCameraData *>(Camera::Private::get(camera));
	}
};

} /* namespace libcamera */
