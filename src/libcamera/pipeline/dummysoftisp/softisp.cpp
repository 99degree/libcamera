/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <thread>
#include <cstdio>
#include <stdio.h>
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
#include <sys/stat.h>
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
	if (empty()) {
			return Invalid;
	}

	/* Adjust and validate */
	Status status = Valid;
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
	dummyStream_ = std::make_unique<Stream>();
}

DummySoftISPCameraData::~DummySoftISPCameraData()
{
	exit(0); wait();
}

int DummySoftISPCameraData::init()
{
	LOG(SoftISPDummyPipeline, Info) << "Initializing SoftISP camera (virtual)";
	
	/*
	 * Attempt to load the SoftISP IPA module.
	 * 
	 * Note: In Termux environments, this may fail with "Failed to fork: Invalid argument"
	 * because the default IPA proxy mechanism requires process isolation via fork().
	 * If this occurs, the pipeline will continue without IPA processing.
	 */
	int ret = loadIPA();
	if (ret < 0) {
		LOG(SoftISPDummyPipeline, Error) << "Failed to load IPA module (expected in Termux without proxy fix)";
		LOG(SoftISPDummyPipeline, Info) << "Continuing without IPA processing...";
		// Return success anyway to allow testing buffer/request flow without IPA
		return 0;
	}
	
	LOG(SoftISPDummyPipeline, Info) << "SoftISP IPA module loaded successfully";
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
	/* Load the SoftISP IPA module using standard proxy mechanism */
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
	static bool created = false;
	if (created)
		return false;
	created = true;

	LOG(SoftISPDummyPipeline, Info) << "Creating SoftISP dummy camera";

	/* Create camera data */
	auto cameraData = std::make_unique<DummySoftISPCameraData>(this);
	if (cameraData) {
	} else {
		}
	LOG(SoftISPDummyPipeline, Info) << "cameraData created: " << (cameraData ? "yes" : "no");
	if (!cameraData || cameraData->init()) {
		LOG(SoftISPDummyPipeline, Error) << "Failed to create camera data";
		return false;
	}

	/* Create camera */
	std::string id = "SoftISP Dummy Camera";
	std::set<Stream *> streams;
	if (cameraData && cameraData->dummyStream_) {
		streams.insert(cameraData->dummyStream_.get());
	} else {
	}
	auto camera = Camera::create(std::move(cameraData), id, streams);
	if (camera) {
	}
	LOG(SoftISPDummyPipeline, Info) << "Camera::create() returned: " << (camera ? "valid camera" : "nullptr");
	/* Check the camera state */
	if (camera) {
		LOG(SoftISPDummyPipeline, Info) << "Camera ID: " << camera->id();
	}
	if (!camera) {
		LOG(SoftISPDummyPipeline, Error) << "Failed to create camera";
		return false;
	}

	/* Register camera */
		registerCamera(std::move(camera));
	LOG(SoftISPDummyPipeline, Info) << "Camera registered successfully";
	LOG(SoftISPDummyPipeline, Info) << "Camera registered";

	return true;
}

std::unique_ptr<CameraConfiguration> PipelineHandlerDummysoftisp::generateConfiguration(
	Camera *camera, Span<const StreamRole> roles)
{
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

	/* Load the IPA module - skipped for dummy pipeline */
	// ret = data->loadIPA();
	// if (ret)
	// 	return ret;

	/* Configure the IPA module */
	if (data->ipa_) {
		/* Call IPA init */
		int32_t ret = data->ipa_->init(IPASettings{}, SharedFD{}, SharedFD{},
					   IPACameraSensorInfo{}, ControlInfoMap{}, nullptr, nullptr);
		if (ret < 0) {
			LOG(SoftISPDummyPipeline, Error) << "Failed to init IPA: " << ret;
			return ret;
		}
		LOG(SoftISPDummyPipeline, Info) << "IPA init successful";
		
		/* Call IPA configure */
		ipa::soft::IPAConfigInfo configInfo{};
		ret = data->ipa_->configure(configInfo);
		if (ret < 0) {
			LOG(SoftISPDummyPipeline, Error) << "Failed to configure IPA: " << ret;
			return ret;
		}
		LOG(SoftISPDummyPipeline, Info) << "IPA configure successful";
	} else {
		LOG(SoftISPDummyPipeline, Warning) << "IPA not loaded, skipping init/configure";
	}

	/* Set the stream for each configuration entry */
	for (auto it = config->begin(); it != config->end(); ++it) {
		StreamConfiguration &cfg = *it;
				LOG(SoftISPDummyPipeline, Info) << "Configuring stream " << std::distance(config->begin(), it) << ": " << cfg.size.width << "x" << cfg.size.height << "-" << cfg.pixelFormat.toString();
		if (data->dummyStream_) {
			cfg.setStream(data->dummyStream_.get());
			LOG(SoftISPDummyPipeline, Info) << "Configured stream: " << cfg.size.toString() << "-" << cfg.pixelFormat.toString();
		} else {
					LOG(SoftISPDummyPipeline, Error) << "No dummy stream available!";
			return -EINVAL;
		}
	}
	return 0;
}

int PipelineHandlerDummysoftisp::exportFrameBuffers(Camera *camera, Stream *stream, std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	if (!stream || !buffers) {
		fprintf(stderr, "ERROR [exportFrameBuffers]: Invalid parameters\n");
		return -EINVAL;
	}

	const StreamConfiguration &config = stream->configuration();
	const PixelFormatInfo &info = PixelFormatInfo::info(config.pixelFormat);

	/* Calculate buffer size for each plane */
	std::vector<uint32_t> planeSizes;
	uint32_t totalSize = 0;
	for (size_t i = 0; i < info.numPlanes(); ++i) {
		uint32_t size = info.planeSize(config.size, i);
		planeSizes.push_back(size);
		totalSize += size;
	}

	buffers->clear();
	DummySoftISPCameraData *data = cameraData(camera);

	/* Iteratively create each buffer */
	for (unsigned int i = 0; i < config.bufferCount; ++i) {

		/* Try memfd first */
		int fd = memfd_create("softisp_buf", MFD_CLOEXEC);
		if (fd < 0) {
			fprintf(stderr, "WARN [exportFrameBuffers]: memfd_create failed (%d), trying mkstemp\n", errno);
			/* Fallback: mkstemp */
			char tmpname[] = "/tmp/softisp_XXXXXX";
			fd = mkstemp(tmpname);
			if (fd < 0) {
				fprintf(stderr, "ERROR [exportFrameBuffers]: mkstemp failed: %d\n", errno);
				return -errno;
			}
			unlink(tmpname); /* Unlink immediately */
			if (ftruncate(fd, totalSize) < 0) {
				fprintf(stderr, "ERROR [exportFrameBuffers]: ftruncate (mkstemp) failed: %d\n", errno);
				close(fd);
				return -errno;
			}
		} else {
			/* memfd succeeded, set size */
			if (ftruncate(fd, totalSize) < 0) {
				fprintf(stderr, "ERROR [exportFrameBuffers]: ftruncate (memfd) failed: %d\n", errno);
				close(fd);
				return -errno;
			}
		}

		/* Map and initialize */
		void *map = mmap(nullptr, totalSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (map == MAP_FAILED) {
			fprintf(stderr, "ERROR [exportFrameBuffers]: mmap failed: %d\n", errno);
			close(fd);
			return -errno;
		}
		memset(map, 0x80, totalSize); /* Gray pattern */
		munmap(map, totalSize);

		/* Build planes vector */
		std::vector<FrameBuffer::Plane> planes;
		uint32_t offset = 0;
		for (size_t p = 0; p < planeSizes.size(); ++p) {
			FrameBuffer::Plane plane;
			plane.fd = SharedFD(fd); /* Each plane references the same fd */
			plane.length = planeSizes[p];
			plane.offset = offset;
			planes.push_back(plane);
			offset += planeSizes[p];
		}

		/* Create FrameBuffer using Span */
		auto buffer = std::unique_ptr<FrameBuffer>(new FrameBuffer(libcamera::Span<const FrameBuffer::Plane>(planes)));

		buffers->push_back(std::move(buffer));

		/* Note: buffers are owned by the caller (FrameBufferAllocator) */
	}

	return 0;
}

int PipelineHandlerDummysoftisp::start(Camera *camera, const ControlList *controls)
{
	DummySoftISPCameraData *data = cameraData(camera);
	if (!data) {
			return -EINVAL;
	}

	/* Buffers will be exported automatically by libcamera when needed */

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
	if (!data) {
			return -EINVAL;
	}

	/* Check if request has buffers */
	const auto &buffers = request->buffers();

	/* For dummy pipeline, we can proceed even without buffers */
	if (buffers.empty()) {
			/* request->complete() not available */
		return 0;
	}

	/* Queue the request for processing */
	data->processRequest(request);
	return 0;
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerDummysoftisp, "dummysoftisp")

} /* namespace libcamera */

