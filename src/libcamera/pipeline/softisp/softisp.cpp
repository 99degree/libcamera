/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (virtual cameras)
 */
#include "softisp.h"
#include "virtual_camera.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <queue>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include <libcamera/base/log.h>
#include <libcamera/controls.h>
#include <libcamera/control_ids.h>

#include "libcamera/internal/camera.h"
#include "libcamera/stream.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/request.h"
#include "libcamera/internal/ipa_manager.h"

namespace libcamera {

static std::map<uint32_t, int> g_bufferFdMap;

LOG_DEFINE_CATEGORY(SoftISPPipeline)

bool PipelineHandlerSoftISP::created_ = false;

SoftISPConfiguration::SoftISPConfiguration()
{
}

CameraConfiguration::Status SoftISPConfiguration::validate()
{
	if (empty())
		return Invalid;

	Status status = Valid;
	for (auto it = begin(); it != end(); ++it) {
		StreamConfiguration &cfg = *it;

		if (cfg.size.width == 0 || cfg.size.height == 0)
			return Invalid;
		if (cfg.pixelFormat == 0)
			return Invalid;
	}

	return status;
}

SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
	: Camera::Private(pipe), Thread("SoftISPCamera")
{
	dummyStream_ = std::make_unique<Stream>();
	virtualCamera_ = std::make_unique<VirtualCamera>();
}

SoftISPCameraData::~SoftISPCameraData()
{
	exit(0);
	wait();
}

int SoftISPCameraData::init()
{
	int ret = loadIPA();
	if (ret)
		return ret;

	ret = virtualCamera_->init(1920, 1080);
	if (ret) {
		LOG(SoftISPPipeline, Error) << "Failed to initialize virtual camera";
		return ret;
	}

	return 0;
}

int SoftISPCameraData::loadIPA()
{
	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(
		Camera::Private::pipe(), 0, 0);
	if (!ipa_) {
		LOG(SoftISPPipeline, Info) << "IPA module not available";
		return 0;
	}

	LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded for virtual camera";
	return 0;
}

void SoftISPCameraData::run()
{
	while (running_) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void SoftISPCameraData::processRequest(Request *request)
{
	if (!ipa_) {
		LOG(SoftISPPipeline, Error) << "IPA not initialized";
		pipe()->completeRequest(request);
		return;
	}

	static uint32_t frameCounter = 0;
	uint32_t frameId = frameCounter++;

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

	void *bufferMem = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE,
	                       MAP_SHARED, plane.fd.get(), 0);
	if (bufferMem == MAP_FAILED) {
		LOG(SoftISPPipeline, Error) << "Failed to map buffer";
		pipe()->completeRequest(request);
		return;
	}

	g_bufferFdMap[bufferId] = plane.fd.get();

	const Stream *stream = buffers.begin()->first;
	auto streamConfig = stream->configuration();
	ControlList statsResults;
	
	// Call processStats
	ipa_->processStats(frameId, bufferId, statsResults);

	// Merge results into request metadata
	request->controls().merge(statsResults, libcamera::ControlList::MergePolicy::OverwriteExisting);

	// Call processFrame with bufferFd (async, returns void)
	const ControlList &results = request->controls();
	ipa_->processFrame(frameId, bufferId, plane.fd, 0,
	                   streamConfig.size.width, streamConfig.size.height,
	                   results);


	g_bufferFdMap.erase(bufferId);
	munmap(bufferMem, plane.length);
	bufferMap_.erase(bufferId);

	request->controls().set(
		controls::SensorTimestamp, static_cast<int64_t>(frameId * 33333));
	pipe()->completeRequest(request);
}
}
