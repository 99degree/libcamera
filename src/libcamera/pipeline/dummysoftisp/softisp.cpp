/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (dummy cameras)
 */
#include "softisp.h"
#include "virtual_camera.h"
#include <algorithm>
#include <cstring>
#include <memory>
#include <queue>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include <libcamera/base/log.h>
class DummySoftISPConfiguration;
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

static std::map<uint32_t, int> g_bufferFdMap;

LOG_DEFINE_CATEGORY(SoftISPDummyPipeline)

bool PipelineHandlerDummysoftisp::created_ = false;

DummySoftISPConfiguration::DummySoftISPConfiguration()
{
}

CameraConfiguration::Status DummySoftISPConfiguration::validate()
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

DummySoftISPCameraData::DummySoftISPCameraData(PipelineHandlerDummysoftisp *pipe)
	: Camera::Private(pipe), Thread("SoftISPCamera")
{
	dummyStream_ = std::make_unique<Stream>();
	virtualCamera_ = std::make_unique<VirtualCamera>();
}

DummySoftISPCameraData::~DummySoftISPCameraData()
{
	exit(0);
	wait();
}

int DummySoftISPCameraData::init()
{
	int ret = loadIPA();
	if (ret)
		return ret;

	ret = virtualCamera_->init(1920, 1080);
	if (ret) {
		LOG(SoftISPDummyPipeline, Error) << "Failed to initialize virtual camera";
		return ret;
	}

	return 0;
}

int DummySoftISPCameraData::loadIPA()
{
	int ret = IPAManager::createIPA<ipa::soft::IPAProxySoft>(
		Camera::Private::pipe(), 0, 0, &ipa_);
	if (ret) {
		LOG(SoftISPDummyPipeline, Warn) << "Failed to load IPA module";
		return ret;
	}

	LOG(SoftISPDummyPipeline, Info) << "SoftISP IPA module loaded for virtual camera";
	return 0;
}

void DummySoftISPCameraData::run()
{
	while (running_) {
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

FrameBuffer* DummySoftISPCameraData::getBufferFromId(uint32_t bufferId)
{
	auto it = bufferMap_.find(bufferId);
	return (it != bufferMap_.end()) ? it->second : nullptr;
}

void DummySoftISPCameraData::processRequest(Request *request)
{
	if (!ipa_) {
		LOG(SoftISPDummyPipeline, Error) << "IPA not initialized";
		pipe()->completeRequest(request);
		return;
	}

	static uint32_t frameCounter = 0;
	uint32_t frameId = frameCounter++;

	const auto &buffers = request->buffers();
	if (buffers.empty()) {
		LOG(SoftISPDummyPipeline, Error) << "No buffers in request";
		pipe()->completeRequest(request);
		return;
	}

	FrameBuffer *buffer = buffers.begin()->second;
	if (!buffer || buffer->planes().empty()) {
		LOG(SoftISPDummyPipeline, Error) << "Invalid buffer";
		pipe()->completeRequest(request);
		return;
	}

	uint32_t bufferId = static_cast<uint32_t>(buffer->planes()[0].fd.get());
	const auto &plane = buffer->planes()[0];

	void *bufferMem = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE,
	                       MAP_SHARED, plane.fd.get(), 0);
	if (bufferMem == MAP_FAILED) {
		LOG(SoftISPDummyPipeline, Error) << "Failed to map buffer";
		pipe()->completeRequest(request);
		return;
	}

	g_bufferFdMap[bufferId] = plane.fd.get();

	const Stream *stream = buffers.begin()->first;
	auto streamConfig = stream->configuration();
	ControlList statsResults;
	ipa_->processStats(frameId, bufferId, plane.fd,
	                   streamConfig.size.width, streamConfig.size.height,
	                   statsResults);

	for (const auto &ctrl : statsResults) {
		request->metadata().add(ctrl);
	}

	int32_t ret = ipa_->processFrame(frameId, bufferId, plane.fd, plane.fd,
	                                 streamConfig.size.width, streamConfig.size.height,
	                                 &request->metadata());
	if (ret != 0) {
		LOG(SoftISPDummyPipeline, Error) << "processFrame failed with error " << ret;
	}

	g_bufferFdMap.erase(bufferId);
	munmap(bufferMem, plane.length);
	bufferMap_.erase(bufferId);

	const_cast<ControlList &>(request->metadata()).set(
		controls::SensorTimestamp, static_cast<int64_t>(frameId * 33333));
	pipe()->completeRequest(request);
}

PipelineHandlerDummysoftisp::PipelineHandlerDummysoftisp(CameraManager *manager)
	: PipelineHandler(manager)
{
}

PipelineHandlerDummysoftisp::~PipelineHandlerDummysoftisp()
{
}

std::unique_ptr<CameraConfiguration>
PipelineHandlerDummysoftisp::generateConfiguration(Camera *camera,
                                                   Span<const StreamRole> roles)
{
	(void)camera;
	if (roles.empty())
		return nullptr;

	auto config = std::make_unique<DummySoftISPConfiguration>();
	config->addStream(StreamRole::Viewfinder, Size(1920, 1080), formats::UYVY888);

	return config;
}

int PipelineHandlerDummysoftisp::configure(Camera *camera,
                                           CameraConfiguration *config)
{
	(void)config;
	DummySoftISPCameraData *data = cameraData(camera);

	int ret = data->init();
	if (ret)
		return ret;

	ret = data->virtualCamera_->start();
	if (ret) {
		LOG(SoftISPDummyPipeline, Error) << "Failed to start virtual camera";
		return ret;
	}

	data->running_ = true;
	data->start();

	return 0;
}

int PipelineHandlerDummysoftisp::exportFrameBuffers(Camera *camera,
                                                    Stream *stream,
                                                    std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	DummySoftISPCameraData *data = cameraData(camera);

	unsigned int numBuffers = 4;
	unsigned int width = stream->configuration().size.width;
	unsigned int height = stream->configuration().size.height;
	unsigned int size = width * height * 2;

	for (unsigned int i = 0; i < numBuffers; i++) {
		std::unique_ptr<FrameBuffer> buffer;
		int ret = dmaBufAllocator_.exportBuffers(1, size, &buffer);
		if (ret) {
			LOG(SoftISPDummyPipeline, Error) << "Failed to allocate buffer " << i;
			return -ENOMEM;
		}

		buffers->push_back(std::move(buffer));
	}

	return 0;
}

int PipelineHandlerDummysoftisp::start(Camera *camera, const ControlList *controls)
{
	(void)camera;
	(void)controls;
	return 0;
}

void PipelineHandlerDummysoftisp::stopDevice(Camera *camera)
{
	DummySoftISPCameraData *data = cameraData(camera);

	data->running_ = false;
	data->virtualCamera_->stop();
	data->exit(0);
	data->wait();
}

int PipelineHandlerDummysoftisp::queueRequestDevice(Camera *camera, Request *request)
{
	DummySoftISPCameraData *data = cameraData(camera);

	const auto &buffers = request->buffers();
	if (buffers.empty())
		return -EINVAL;

	FrameBuffer *buffer = buffers.begin()->second;
	if (!buffer || buffer->planes().empty())
		return -EINVAL;

	data->virtualCamera_->queueBuffer(buffer);
	data->processRequest(request);

	return 0;
}

bool PipelineHandlerDummysoftisp::match(DeviceEnumerator *enumerator)
{
	(void)enumerator;
	LOG(SoftISPDummyPipeline, Info) << "Registering as virtual camera";
	registerCamera(std::make_unique<Camera>(this));
	created_ = true;
	return true;
}

REGISTER_PIPELINE_HANDLER(PipelineHandlerDummysoftisp, "dummysoftisp")

} /* namespace libcamera */
