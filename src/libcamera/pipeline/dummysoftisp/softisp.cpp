/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <thread>
#include <cstdio>
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
#include "libcamera/stream.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/camera_sensor.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include <libcamera/formats.h>
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"
#include "libcamera/internal/v4l2_subdevice.h"
#include "libcamera/internal/v4l2_videodevice.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/base/object.h"

namespace libcamera {


/* DummySoftISPConfiguration class declaration */
class DummySoftISPConfiguration : public CameraConfiguration {
public:
	DummySoftISPConfiguration();
	Status validate() override;
};

LOG_DEFINE_CATEGORY(SoftISPDummyPipeline)

/* -----------------------------------------------------------------------------
 * DummySoftISPConfiguration Implementation
 * ---------------------------------------------------------------------------*/

bool PipelineHandlerDummysoftisp::created_ = false;

DummySoftISPConfiguration::DummySoftISPConfiguration() {
}

CameraConfiguration::Status DummySoftISPConfiguration::validate() {
	fprintf(stderr, "DEBUG: DummySoftISPConfiguration::validate() called, empty=%d\n", empty());
	if (empty()) {
		return Invalid;
	}

	/* Adjust and validate */
	Status status = Adjusted;
	for (auto it = begin(); it != end(); ++it) {
		StreamConfiguration &cfg = *it;
		/* Validate stream configuration */
		if (cfg.size.width == 0 || cfg.size.height == 0)
			return Invalid;
		if (cfg.pixelFormat == 0)
			return Invalid;
	}

	return status;
}


/* -----------------------------------------------------------------------------
 * DummySoftISPCameraData Implementation
 * ---------------------------------------------------------------------------*/

DummySoftISPCameraData::DummySoftISPCameraData(PipelineHandlerDummysoftisp *pipe)
	: Camera::Private(pipe), Thread("SoftISPCamera")
{
	fprintf(stderr, "DEBUG [ctor]: Creating dummy stream\n");
	dummyStream_ = std::make_unique<Stream>();
	fprintf(stderr, "DEBUG [ctor]: dummyStream_ = %p\n", static_cast<void*>(dummyStream_.get()));
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
	fprintf(stderr, "=== DEBUG [match]: ENTERED match() ===\n");
	fprintf(stderr, "DEBUG [match]: this (pipeline) = %p\n", static_cast<void*>(this));
	fprintf(stderr, "DEBUG [match]: static created flag = %d\n", created_);
	static bool created = false;
	if (created)
		return false;
	created = true;

	LOG(SoftISPDummyPipeline, Info) << "Creating SoftISP dummy camera";

	/* Create camera data */
	fprintf(stderr, "DEBUG [match]: About to create cameraData\n");
	auto cameraData = std::make_unique<DummySoftISPCameraData>(this);
	fprintf(stderr, "DEBUG [match]: cameraData created: %p\n", static_cast<void*>(cameraData.get()));
	if (cameraData) {
		fprintf(stderr, "DEBUG [match]: cameraData->dummyStream_ = %p\n", static_cast<void*>(cameraData->dummyStream_.get()));
		fprintf(stderr, "DEBUG [match]: cameraData->dummyStream_ is null? %s\n", cameraData->dummyStream_ ? "NO" : "YES");
	} else {
		fprintf(stderr, "DEBUG [match]: cameraData is nullptr!\n");
	}
	LOG(SoftISPDummyPipeline, Info) << "cameraData created: " << (cameraData ? "yes" : "no");
	if (!cameraData || cameraData->init()) {
		LOG(SoftISPDummyPipeline, Error) << "Failed to create camera data";
		return false;
	}

	/* Create camera */
	std::string id = "SoftISP Dummy Camera";
	std::set<Stream *> streams;
	fprintf(stderr, "DEBUG [match]: About to insert stream\n");
	if (cameraData && cameraData->dummyStream_) {
		fprintf(stderr, "DEBUG [match]: Inserting dummy stream %p into streams set\n", static_cast<void*>(cameraData->dummyStream_.get()));
		streams.insert(cameraData->dummyStream_.get());
		fprintf(stderr, "DEBUG [match]: streams set size after insert = %zu\n", streams.size());
	} else {
		fprintf(stderr, "DEBUG [match]: dummyStream_ is nullptr! cameraData=%p\n", static_cast<void*>(cameraData.get()));
	}
	fprintf(stderr, "DEBUG [match]: About to call Camera::create with streams.size() = %zu\n", streams.size());
	auto camera = Camera::create(std::move(cameraData), id, streams);
	fprintf(stderr, "DEBUG [match]: Camera::create returned: %p\n", static_cast<void*>(camera.get()));
	if (camera) {
		fprintf(stderr, "DEBUG [match]: camera->streams().size() = %zu\n", camera->streams().size());
	}
	LOG(SoftISPDummyPipeline, Info) << "Camera::create() returned: " << (camera ? "valid camera" : "nullptr");
	fprintf(stderr, "DEBUG [match]: Created Camera Object Address: %p\n", static_cast<void*>(camera.get()));
	fprintf(stderr, "DEBUG [match]: camera->_d() = %p\n", static_cast<void*>(camera->_d()));
	fprintf(stderr, "DEBUG [match]: camera->_d()->pipe() = %p\n", static_cast<void*>(camera->_d()->pipe()));
	fprintf(stderr, "DEBUG [match]: this (pipeline) = %p\n", static_cast<void*>(this));
	fprintf(stderr, "DEBUG [match]: pipe match? %s\n", (camera->_d()->pipe() == this) ? "YES" : "NO");
	/* Check the camera state */
	fprintf(stderr, "DEBUG [match]: Camera state check: trying to call generateConfiguration\n");
	fprintf(stderr, "DEBUG [match]: camera->streams().size() = %zu\n", camera->streams().size());
	fprintf(stderr, "DEBUG [match]: roles.size() = 1\n");
	if (camera) {
		LOG(SoftISPDummyPipeline, Info) << "Camera ID: " << camera->id();
	}
	if (!camera) {
		LOG(SoftISPDummyPipeline, Error) << "Failed to create camera";
		return false;
	}

	/* Register camera */
		fprintf(stderr, "DEBUG [match]: About to call registerCamera\n");
	registerCamera(std::move(camera));
	fprintf(stderr, "DEBUG [match]: registerCamera completed\n");
	LOG(SoftISPDummyPipeline, Info) << "Camera registered successfully";
	LOG(SoftISPDummyPipeline, Info) << "Camera registered";

fprintf(stderr, "DEBUG [match]: Returning true from match()\n");
	fprintf(stderr, "=== DEBUG [match]: EXITING match() ===\n");
	return true;
}

std::unique_ptr<CameraConfiguration> PipelineHandlerDummysoftisp::generateConfiguration(
	Camera *camera, Span<const StreamRole> roles)
{
	LOG(SoftISPDummyPipeline, Info) << ">>> generateConfiguration called for camera: " << camera->id() << " with " << roles.size() << " roles";
	DummySoftISPCameraData *data = cameraData(camera);
	if (!data) {
		LOG(SoftISPDummyPipeline, Error) << "No camera data!";
		return nullptr;
	}
	if (roles.empty()) {
		LOG(SoftISPDummyPipeline, Error) << "No roles!";
		return nullptr;
	}
	if (!data || roles.empty())
		return nullptr;

	auto config = std::make_unique<DummySoftISPConfiguration>();

	/* Use formats::NV12 which is known to work */
	std::map<PixelFormat, std::vector<SizeRange>> streamFormats;
	streamFormats[formats::NV12] = { SizeRange(libcamera::Size(640, 480), Size(1920, 1080)) };
	
	StreamFormats sf(streamFormats);
	StreamConfiguration streamCfg(sf);
	streamCfg.pixelFormat = formats::NV12;
	streamCfg.size = Size(640, 480);
	streamCfg.bufferCount = 4;

	config->addConfiguration(streamCfg);

	if (config->validate() == CameraConfiguration::Invalid)
		return nullptr;

	return config;
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

