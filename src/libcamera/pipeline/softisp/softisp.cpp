/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (real cameras)
 */
#include "softisp.h"

#include <algorithm>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <libcamera/base/log.h>
#include <libcamera/controls.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"
#include "libcamera/internal/ipa_manager.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPPipeline)

/* Global map for buffer FDs */
static std::map<uint32_t, int> g_bufferFdMap;

/* -----------------------------------------------------------------------------
 * SoftISPCameraData Implementation
 * ---------------------------------------------------------------------------*/
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
		LOG(SoftISPPipeline, Error) << "Failed to create SoftISP IPA module for real camera";
		return -ENOENT;
	}
	LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded for real camera";
	return 0;
}

void SoftISPCameraData::processRequest(Request *request)
{
	LOG(SoftISPPipeline, Info) << "[DEBUG] processRequest() START (real camera)";
	
	if (!ipa_) {
		LOG(SoftISPPipeline, Error) << "IPA not initialized";
		pipe()->completeRequest(request);
		return;
	}
	
	try {
		/* Use frame sequence number */
		uint32_t frameId = request->sequence();

		/* Get the buffer from the request */
		const auto &buffers = request->buffers();
		if (buffers.empty()) {
			LOG(SoftISPPipeline, Error) << "No buffers in request";
			pipe()->completeRequest(request);
			return;
		}
		
		FrameBuffer *buffer = buffers.begin()->second;
		if (!buffer || buffer->planes().empty()) {
			LOG(SoftISPPipeline, Error) << "Invalid buffer";
			pipe()->completeRequest(request);
			return;
		}

		uint32_t bufferId = static_cast<uint32_t>(buffer->planes()[0].fd.get());
		const auto &plane = buffer->planes()[0];
		
		/* Map memory for IPA */
		void *bufferMem = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE,
				       MAP_SHARED, plane.fd.get(), 0);
		if (bufferMem == MAP_FAILED) {
			LOG(SoftISPPipeline, Error) << "Failed to map buffer";
			pipe()->completeRequest(request);
			return;
		}

		g_bufferFdMap[bufferId] = plane.fd.get();

		/* Call IPA processStats to run ONNX inference */
		LOG(SoftISPPipeline, Info) << "[DEBUG] Calling ipa_->processStats() for frame " << frameId;
		ipa_->processStats(frameId, bufferId, ControlList{});

		/* Call IPA processFrame to apply results to buffer */
		LOG(SoftISPPipeline, Info) << "[DEBUG] Calling ipa_->processFrame() for frame " << frameId;
		auto streamConfig = buffer->stream()->configuration();
		int32_t ret = ipa_->processFrame(frameId, bufferId, plane.fd, 0, plane.stride,
						 streamConfig.size.width, streamConfig.size.height,
						 &request->metadata());
		if (ret != 0) {
			LOG(SoftISPPipeline, Error) << "processFrame failed with error " << ret;
		}
		LOG(SoftISPPipeline, Info) << "[DEBUG] ipa_->processFrame() completed";

		/* Unmap buffer */
		g_bufferFdMap.erase(bufferId);
		munmap(bufferMem, plane.length);

		/* Set metadata and complete request */
		request->metadata().set(controls::SensorTimestamp, static_cast<int64_t>(frameId * 33333));
		LOG(SoftISPPipeline, Info) << "[DEBUG] Completing request for frame " << frameId;
		pipe()->completeRequest(request);
		LOG(SoftISPPipeline, Info) << "[DEBUG] Request completed for frame " << frameId;
		
	} catch (const std::exception& e) {
		LOG(SoftISPPipeline, Error) << "Error processing request: " << e.what();
		pipe()->completeRequest(request);
	}
	
	LOG(SoftISPPipeline, Info) << "[DEBUG] processRequest() END";
}

/* -----------------------------------------------------------------------------
 * PipelineHandlerSoftISP Implementation
 * ---------------------------------------------------------------------------*/
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
		return false; /* No real camera, don't match */
	}
	return true;
}

std::unique_ptr<CameraConfiguration>
PipelineHandlerSoftISP::generateConfiguration(Camera *camera, Span<const StreamRole> roles)
{
	return nullptr;
}

int PipelineHandlerSoftISP::configure(Camera *camera, CameraConfiguration *config)
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
	SoftISPCameraData *data = cameraData(camera);
	data->processRequest(request);
	return 0;
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "softisp")

} /* namespace libcamera */
