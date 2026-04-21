/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <thread>
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (virtual cameras)
 */

#include "softisp.h"

#include <algorithm>
#include <cstring>
#include <memory>
#include <queue>

#include <libcamera/base/log.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/property_ids.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/camera_sensor.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"
#include "libcamera/internal/v4l2_subdevice.h"
#include "libcamera/internal/v4l2_videodevice.h"
#include "libcamera/internal/ipa_manager.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPPipeline)

/* -----------------------------------------------------------------------------
 * SoftISPCameraData Implementation
 * ---------------------------------------------------------------------------*/

SoftISPCameraData::SoftISPCameraData(PipelineHandlerDummysoftisp *pipe)
	: Camera::Private(pipe), Thread("SoftISPCamera")
{
}

SoftISPCameraData::~SoftISPCameraData()
{
	stop();
}

int SoftISPCameraData::init()
{
	LOG(SoftISPPipeline, Debug) << "Initializing SoftISP camera (virtual)";
	return 0;
}

int SoftISPCameraData::loadIPA()
{
	/*
	 * Load the SoftISP IPA module.
	 *
	 * The IPAManager will search for an IPA module where:
	 * - pipelineName matches the pipeline handler name ("dummysoftisp")
	 * - pipelineVersion is within the specified range (0, 0 = any)
	 */
	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoft>(this->pipe(), 0, 0);
	if (!ipa_) {
		LOG(SoftISPPipeline, Error)
			<< "Failed to create SoftISP IPA module for virtual camera";
		return -ENOENT;
	}

	LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded for virtual camera";

	return 0;
}

void SoftISPCameraData::run()
{
	/*
	 * Thread loop for processing requests.
	 * In a full implementation, this would process incoming requests
	 * and coordinate with the IPA module.
	 */
	while (running_) {
		/* Wait for requests to be processed */
		/* This is a placeholder for the actual implementation */
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void SoftISPCameraData::processRequest(Request *request)
{
	/*
	 * Process a single request through the SoftISP pipeline.
	 *
	 * This would typically:
	 * 1. Extract statistics from the captured frame
	 * 2. Pass statistics to the IPA module
	 * 3. Get metadata (gains, coefficients) from the IPA
	 * 4. Apply metadata to the frame
	 * 5. Complete the request
	 */

	if (!ipa_) {
		LOG(SoftISPPipeline, Error) << "IPA module not loaded";
		return;
	}

	/*
	 * Placeholder for actual IPA processing.
	 * In a full implementation, this would call the IPA's process() method
	 * with the frame statistics and receive the processed metadata.
	 */

	LOG(SoftISPPipeline, Debug) << "Processing request through SoftISP";

	/* Mark request as complete */
	this->pipe()->completeRequest(request);
}

/* -----------------------------------------------------------------------------
 * PipelineHandlerDummysoftisp Implementation
 * ---------------------------------------------------------------------------*/

PipelineHandlerDummysoftisp::PipelineHandlerDummysoftisp(CameraManager *manager)
	: PipelineHandler(manager)
{
	LOG(SoftISPPipeline, Info) << "SoftISP virtual pipeline handler initialized";
}

PipelineHandlerDummysoftisp::~PipelineHandlerDummysoftisp()
{
}

bool PipelineHandlerDummysoftisp::match(DeviceEnumerator *enumerator)
{
	/*
	 * For now, we don't automatically match any devices.
	 * The SoftISP virtual pipeline creates dummy cameras internally.
	 */

	LOG(SoftISPPipeline, Debug) << "SoftISP virtual pipeline handler match() called";
	return true;
}

std::unique_ptr<CameraConfiguration>
PipelineHandlerDummysoftisp::generateConfiguration(Camera *camera,
					      Span<const StreamRole> roles)
{
	SoftISPCameraData *data = cameraData(camera);

	/*
	 * Generate a camera configuration based on the requested roles.
	 * This is a simplified implementation that assumes a single stream.
	 */

	if (roles.empty())
		return nullptr;

	/* Create a basic configuration */
	auto config = std::make_unique<CameraConfiguration>();

	/* For now, return a minimal configuration */
	/* In a full implementation, this would query the sensor capabilities */

	return config;
}

int PipelineHandlerDummysoftisp::configure(Camera *camera,
				      CameraConfiguration *config)
{
	SoftISPCameraData *data = cameraData(camera);

	/*
	 * Configure the camera with the given configuration.
	 * This would typically:
	 * 1. Validate the configuration
	 * 2. Set up streams
	 * 3. Configure the IPA module
	 */

	/* Initialize the camera data */
	int ret = data->init();
	if (ret)
		return ret;

	/* Load the IPA module */
	ret = data->loadIPA();
	if (ret)
		return ret;

	/* Configure the IPA module */
	/* Placeholder for IPA configuration */

	return 0;
}

int PipelineHandlerDummysoftisp::exportFrameBuffers(Camera *camera, Stream *stream,
					       std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	/*
	 * Export frame buffers for the given stream.
	 * This would typically allocate DMA buffers and return them.
	 */

	unsigned int count = stream->configuration().bufferCount;

	for (unsigned int i = 0; i < count; ++i) {
		/* Allocate a buffer */
		std::unique_ptr<FrameBuffer> buffer;
		/* Placeholder for buffer allocation */
		buffers->push_back(std::move(buffer));
	}

	return 0;
}

int PipelineHandlerDummysoftisp::start(Camera *camera, const ControlList *controls)
{
	SoftISPCameraData *data = cameraData(camera);

	/* Start the camera processing thread */
	data->running_ = true;
	data->start();

	LOG(SoftISPPipeline, Info) << "SoftISP virtual camera started";

	return 0;
}

void PipelineHandlerDummysoftisp::stopDevice(Camera *camera)
{
	SoftISPCameraData *data = cameraData(camera);

	/* Stop the camera processing thread */
	data->running_ = false;
	Thread::stop();

	LOG(SoftISPPipeline, Info) << "SoftISP virtual camera stopped";
}

int PipelineHandlerDummysoftisp::queueRequestDevice(Camera *camera, Request *request)
{
	SoftISPCameraData *data = cameraData(camera);

	/* Queue the request for processing */
	/* In a full implementation, this would add the request to a queue
	 * and signal the processing thread */

	data->processRequest(request);

	return 0;
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerDummysoftisp, "dummysoftisp")
} /* namespace libcamera */
