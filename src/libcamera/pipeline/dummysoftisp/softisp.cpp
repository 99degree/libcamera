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
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

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

LOG_DEFINE_CATEGORY(SoftISPDummyPipeline)

/* -----------------------------------------------------------------------------
 * DummySoftISPCameraData Implementation
 * ---------------------------------------------------------------------------*/

DummySoftISPCameraData::DummySoftISPCameraData(PipelineHandlerDummysoftisp *pipe)
	: Camera::Private(pipe), Thread("SoftISPCamera")
{
}

DummySoftISPCameraData::~DummySoftISPCameraData()
{
	exit(0); wait();
}

int DummySoftISPCameraData::init()
{
	LOG(SoftISPDummyPipeline, Debug) << "Initializing SoftISP camera (virtual)";
	return 0;
}

int DummySoftISPCameraData::loadIPA()
{
	/*
	 * Load the SoftISP IPA module.
	 *
	 * The IPAManager will search for an IPA module where:
	 * - pipelineName matches the pipeline handler name ("dummysoftisp")
	 * - pipelineVersion is within the specified range (0, 0 = any)
	 */
	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoft>(Camera::Private::pipe(), 0, 0);
	if (!ipa_) {
		LOG(SoftISPDummyPipeline, Error)
			<< "Failed to create SoftISP IPA module for virtual camera";
		return -ENOENT;
	}

	LOG(SoftISPDummyPipeline, Info) << "SoftISP IPA module loaded for virtual camera";

	return 0;
}

void DummySoftISPCameraData::run()
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

void DummySoftISPCameraData::processRequest(Request *request)
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
		LOG(SoftISPDummyPipeline, Error) << "IPA module not loaded";
		return;
	}

	/*
	 * Placeholder for actual IPA processing.
	 * In a full implementation, this would call the IPA's process() method
	 * with the frame statistics and receive the processed metadata.
	 */

	LOG(SoftISPDummyPipeline, Debug) << "Processing request through SoftISP";

	/* Mark request as complete */
	Camera::Private::pipe()->completeRequest(request);
}

/* -----------------------------------------------------------------------------
 * PipelineHandlerDummysoftisp Implementation
 * ---------------------------------------------------------------------------*/

PipelineHandlerDummysoftisp::PipelineHandlerDummysoftisp(CameraManager *manager)
	: PipelineHandler(manager)
{
	LOG(SoftISPDummyPipeline, Info) << "SoftISP virtual pipeline handler initialized";
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

	LOG(SoftISPDummyPipeline, Debug) << "SoftISP virtual pipeline handler match() called";
	return true;
}

std::unique_ptr<CameraConfiguration>
PipelineHandlerDummysoftisp::generateConfiguration(Camera *camera,
					      Span<const StreamRole> roles)
{
	DummySoftISPCameraData *data = cameraData(camera);

	/*
	 * Generate a camera configuration based on the requested roles.
	 * This is a simplified implementation that assumes a single stream.
	 */

	if (roles.empty())
		return nullptr;

	/* Create a basic configuration */
	return nullptr; // Placeholder: Not implemented yet

	/* For now, return a minimal configuration */
	/* In a full implementation, this would query the sensor capabilities */

	return nullptr;
}

int PipelineHandlerDummysoftisp::configure(Camera *camera,
				      CameraConfiguration *config)
{
	DummySoftISPCameraData *data = cameraData(camera);

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
	DummySoftISPCameraData *data = cameraData(camera);

	/* Start the camera processing thread */
	data->running_ = true;
	data->start();

	LOG(SoftISPDummyPipeline, Info) << "SoftISP virtual camera started";

	return 0;
}

void PipelineHandlerDummysoftisp::stopDevice(Camera *camera)
{
	DummySoftISPCameraData *data = cameraData(camera);

	/* Stop the camera processing thread */
	data->running_ = false;
	exit(0);

	LOG(SoftISPDummyPipeline, Info) << "SoftISP virtual camera stopped";
}

int PipelineHandlerDummysoftisp::queueRequestDevice(Camera *camera, Request *request)
{
	DummySoftISPCameraData *data = cameraData(camera);

	/* Queue the request for processing */
	/* In a full implementation, this would add the request to a queue
	 * and signal the processing thread */

	data->processRequest(request);

	return 0;
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerDummysoftisp, "dummysoftisp")

} /* namespace libcamera */
