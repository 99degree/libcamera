/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
 *
 * Pipeline handler for SoftISP (real cameras)
 */

#include "softisp.h"

#include <algorithm>
#include <memory>

#include <libcamera/base/log.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/internal/v4l2_device.h"
#include "libcamera/internal/v4l2_videodevice.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPPipeline)

SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
	: Camera::Private(pipe)
{
}

SoftISPCameraData::~SoftISPCameraData()
{
}

int SoftISPCameraData::init()
{
	LOG(SoftISPPipeline, Debug) << "Initializing SoftISP camera (real)";
	return 0;
}

int SoftISPCameraData::loadIPA()
{
	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoft>(pipe(), 0, 0);
	if (!ipa_) {
		LOG(SoftISPPipeline, Error)
			<< "Failed to create SoftISP IPA module for real camera";
		return -ENOENT;
	}

	LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded for real camera";

	return 0;
}

PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager)
	: PipelineHandler(manager)
{
	LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler (real cameras) initialized";
}

PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
}

bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
	LOG(SoftISPPipeline, Debug) << "Matching devices for SoftISP pipeline (real)";
	return true;
}

std::unique_ptr<CameraConfiguration>
PipelineHandlerSoftISP::generateConfiguration(Camera *camera,
				      Span<const StreamRole> roles)
{
	return nullptr;
}

int PipelineHandlerSoftISP::configure(Camera *camera,
				  CameraConfiguration *config)
{
	SoftISPCameraData *data = cameraData(camera);

	int ret = data->init();
	if (ret)
		return ret;

	ret = data->loadIPA();
	if (ret)
		return ret;

	return 0;
}

int PipelineHandlerSoftISP::exportFrameBuffers(Camera *camera, Stream *stream,
				       std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	return 0;
}

int PipelineHandlerSoftISP::start(Camera *camera, const ControlList *controls)
{
	SoftISPCameraData *data = cameraData(camera);

	if (!data->captureDevice_) {
		LOG(SoftISPPipeline, Error) << "V4L2 device not configured";
		return -EINVAL;
	}

	LOG(SoftISPPipeline, Info) << "Starting V4L2 streaming";

	/* Queue all buffers to the device */
	for (auto &buffer : data->buffers_) {
		int ret = data->captureDevice_->queueBuffer(buffer.get());
		if (ret) {
			LOG(SoftISPPipeline, Error)
				<< "Failed to queue buffer: " << strerror(-ret);
			return ret;
		}
	}

	/* Start streaming */
	int ret = data->captureDevice_->streamOn();
	if (ret) {
		LOG(SoftISPPipeline, Error)
			<< "Failed to start streaming: " << strerror(-ret);
		return ret;
	}

	data->streaming_ = true;
	LOG(SoftISPPipeline, Info) << "V4L2 streaming started";

	return 0;
}

void PipelineHandlerSoftISP::stopDevice(Camera *camera)
{
	SoftISPCameraData *data = cameraData(camera);

	if (!data->captureDevice_ || !data->streaming_)
		return;

	LOG(SoftISPPipeline, Info) << "Stopping V4L2 streaming";

	/* Stop streaming */
	data->captureDevice_->streamOff();
	data->streaming_ = false;

	/* Buffer queue cleared by streamOff */

	LOG(SoftISPPipeline, Info) << "V4L2 streaming stopped";
}
}

int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request)
{
	SoftISPCameraData *data = cameraData(camera);

	if (!data->captureDevice_ || !data->streaming_) {
		LOG(SoftISPPipeline, Error) << "Camera not streaming";
		completeRequest(request);
		return 0;
	}

	/*
	 * In a full implementation:
	 * 1. Get the buffer associated with this request
	 * 2. Queue the buffer to V4L2
	 * 3. Wait for buffer completion (via DQBUF or event)
	 * 4. Process the buffer through the IPA module
	 * 5. Complete the request with metadata
	 */

	/* For now, just complete the request */
	/* This is a placeholder until V4L2 buffer queueing is fully implemented */
	completeRequest(request);

	return 0;
}



REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "softisp")

} /* namespace libcamera */
