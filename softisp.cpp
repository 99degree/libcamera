/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <sys/stat.h>
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
#include "libcamera/controls.h"
#include "libcamera/internal/ipa_manager.h"

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
	/* SoftISP is for real cameras only - check for V4L2 video devices */
	struct stat st;
	if (stat("/dev/video0", &st) != 0) {
		return false;  /* No real camera, don't match */
	}
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
	LOG(SoftISPPipeline, Info) << "SoftISP camera (real) started";
	return 0;
}

void PipelineHandlerSoftISP::stopDevice(Camera *camera)
{
	LOG(SoftISPPipeline, Info) << "SoftISP camera (real) stopped";
}

int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request)
{
	completeRequest(request);
	return 0;
}


REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "softisp")

} /* namespace libcamera */
