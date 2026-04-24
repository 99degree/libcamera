/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftISP Pipeline Handler
 * 
 * Architecture: Async Processing with Callback Pattern (matches rkisp1/ipu3)
 * 
 * Key Points:
 * 1. Pipeline creates FrameBuffer with memfd (FD managed by FrameBuffer)
 * 2. Pipeline tracks pending requests in frameInfo_ map
 * 3. Pipeline calls IPA (async, non-blocking)
 * 4. IPA signals completion via metadataReady callback
 * 5. Pipeline merges metadata, marks metadataProcessed = true
 * 6. Pipeline checks if all stages complete (tryCompleteRequest)
 * 7. If complete: pipe()->completeRequest() → FrameBuffer destroyed → FD closed
 * 
 * No explicit mmap/munmap needed - memfd + SharedFD handles it!
 */
#include "softisp.h"
#include "virtual_camera.h"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <queue>
#include <sys/mman.h>
#include <unistd.h>
#include <libcamera/base/log.h>
#include <libcamera/base/memfd.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/geometry.h>
#include "libcamera/internal/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"

namespace libcamera {
LOG_DEFINE_CATEGORY(SoftISPPipeline)

// Forward declaration
class SoftISPCameraData;

// FrameInfo to track pending requests (matches rkisp1/ipu3 pattern)
struct SoftISPFrameInfo {
	unsigned int frame;
	Request *request;
	FrameBuffer *buffer;
	bool metadataProcessed;
	
	SoftISPFrameInfo(Request *req, FrameBuffer *buf, unsigned int id)
		: frame(id), request(req), buffer(buf), metadataProcessed(false) {}
};

// FrameInfo manager (matches rkisp1/ipu3 pattern)
class SoftISPFrames {
public:
	SoftISPFrames() = default;
	
	SoftISPFrameInfo *create(Request *request, FrameBuffer *buffer, unsigned int frameId);
	SoftISPFrameInfo *find(unsigned int frame);
	int destroy(unsigned int frame);
	
	bool tryCompleteRequest(SoftISPFrameInfo *info);
	
private:
	std::map<unsigned int, std::unique_ptr<SoftISPFrameInfo>> frameInfo_;
};

// SoftISPFrames implementation
SoftISPFrameInfo *SoftISPFrames::create(Request *request, FrameBuffer *buffer, unsigned int frameId)
{
	auto info = std::make_unique<SoftISPFrameInfo>(request, buffer, frameId);
	auto ret = frameInfo_.emplace(frameId, std::move(info));
	return ret.first->second.get();
}

SoftISPFrameInfo *SoftISPFrames::find(unsigned int frame)
{
	auto it = frameInfo_.find(frame);
	if (it == frameInfo_.end())
		return nullptr;
	return it->second.get();
}

int SoftISPFrames::destroy(unsigned int frame)
{
	auto it = frameInfo_.find(frame);
	if (it == frameInfo_.end())
		return -ENOENT;
	
	frameInfo_.erase(it);
	return 0;
}

bool SoftISPFrames::tryCompleteRequest(SoftISPFrameInfo *info)
{
	// Check if all processing stages are complete
	if (!info->metadataProcessed)
		return false;
	
	// All stages complete, request can be completed
	return true;
}

// PipelineHandlerSoftISP implementation
class PipelineHandlerSoftISP : public PipelineHandler {
public:
	PipelineHandlerSoftISP();
	~PipelineHandlerSoftISP() override;

	const char *name() const override { return "SoftISP"; }

	int registerDevices() override;
	int unregisterDevices() override;

	int createCamera(Camera *camera) override;
	void destroyCamera(Camera *camera) override;

private:
	std::unique_ptr<VirtualCamera> virtualCamera_;
};

// SoftISPCameraData implementation
class SoftISPCameraData : public Camera::Private, public Thread {
public:
	SoftISPCameraData(PipelineHandlerSoftISP *pipe);
	~SoftISPCameraData() override;

	int init();
	int loadIPA();
	int exportFrameBuffers(Stream *stream,
	                       std::vector<std::unique_ptr<FrameBuffer>> *buffers) override;
	int queueRequestDevice(Request *request) override;
	int stop() override;

	// Callback (matches rkisp1/ipu3 pattern)
	void metadataReady(unsigned int frame, const ControlList &metadata);

private:
	void run() override;
	void tryCompleteRequest(SoftISPFrameInfo *info);

	PipelineHandlerSoftISP *pipe() const {
		return static_cast<PipelineHandlerSoftISP *>(Camera::Private::pipe());
	}

	std::unique_ptr<VirtualCamera> virtualCamera_;
	ipa::soft::IPAProxySoftIsp ipa_;
	SoftISPFrames frameInfo_;
	std::atomic<bool> running_{ false };
};

// PipelineHandlerSoftISP implementation
PipelineHandlerSoftISP::PipelineHandlerSoftISP()
{
}

PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
}

int PipelineHandlerSoftISP::registerDevices()
{
	virtualCamera_ = std::make_unique<VirtualCamera>();
	int ret = virtualCamera_->init(1920, 1080);
	if (ret) {
		LOG(SoftISPPipeline, Error) << "Failed to initialize virtual camera";
		return ret;
	}
	
	LOG(SoftISPPipeline, Info) << "Virtual camera registered: 1920x1080";
	return 0;
}

int PipelineHandlerSoftISP::unregisterDevices()
{
	virtualCamera_.reset();
	return 0;
}

int PipelineHandlerSoftISP::createCamera(Camera *camera)
{
	std::unique_ptr<SoftISPCameraData> data = std::make_unique<SoftISPCameraData>(this);
	int ret = data->init();
	if (ret)
		return ret;

	camera->setPrivate(std::move(data));

	LOG(SoftISPPipeline, Info) << "Camera created";
	return 0;
}

void PipelineHandlerSoftISP::destroyCamera(Camera *camera)
{
	camera->setPrivate(nullptr);
	LOG(SoftISPPipeline, Info) << "Camera destroyed";
}

// SoftISPCameraData implementation
SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
	: Camera::Private(pipe), Thread("SoftISPCamera")
{
	// Connect IPA callback (matches rkisp1/ipu3 pattern)
	if (ipa_) {
		ipa_->metadataReady.connect(this, &SoftISPCameraData::metadataReady);
	}
}

SoftISPCameraData::~SoftISPCameraData()
{
	Thread::exit(false);
	wait();
}

int SoftISPCameraData::init()
{
	int ret = loadIPA();
	if (ret)
		return ret;

	return 0;
}

int SoftISPCameraData::loadIPA()
{
	// Try to load IPA module
	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(
		this->pipe(), 0, 0);
	
	if (!ipa_) {
		LOG(SoftISPPipeline, Info) << "IPA module not available, using SoftIsp directly";
		try {
			ipa_ = new ipa::soft::SoftIsp();
			LOG(SoftISPPipeline, Info) << "SoftIsp (ONNX) initialized directly";
			return 0;
		} catch (const std::exception &e) {
			LOG(SoftISPPipeline, Error) << "Failed to initialize SoftIsp: " << e.what();
			return -EINVAL;
		}
	}
	
	LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded";
	return 0;
}

int SoftISPCameraData::exportFrameBuffers(Stream *stream,
                                          std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	if (!buffers) {
		LOG(SoftISPPipeline, Error) << "Null buffers vector";
		return -EINVAL;
	}

	// Get buffer count and size from virtual camera
	unsigned int count = virtualCamera_->bufferCount();
	unsigned int width = 1920;
	unsigned int height = 1080;
	
	// Calculate buffer size for SBGGR10 (10 bits per pixel, packed)
	size_t bufferSize = ((width * 10 + 7) / 8) * height;
	
	LOG(SoftISPPipeline, Info) << "Exporting " << count << " buffers of size " 
	                           << bufferSize << " for " << width << "x" << height;

	buffers->clear();
	for (unsigned int i = 0; i < count; ++i) {
		// Create memfd buffer (matches libcamera pattern)
		UniqueFD fd = MemFd::create("softisp_buffer", bufferSize);
		if (fd.get() < 0) {
			LOG(SoftISPPipeline, Error) << "Failed to create buffer " << i;
			return -errno;
		}

		// Create FrameBuffer with the FD
		FrameBuffer::Plane plane;
		plane.fd = std::move(fd);
		plane.offset = 0;
		plane.length = bufferSize;

		std::vector<FrameBuffer::Plane> planes;
		planes.push_back(std::move(plane));

		auto buffer = std::make_unique<FrameBuffer>(
			Span<const FrameBuffer::Plane>(planes),
			static_cast<unsigned int>(buffers->size())
		);

		buffers->push_back(std::move(buffer));
	}

	LOG(SoftISPPipeline, Info) << "Successfully exported " << buffers->size() << " buffers";
	return 0;
}

int SoftISPCameraData::queueRequestDevice(Request *request)
{
	if (!ipa_) {
		this->pipe()->completeRequest(request);
		return 0;
	}

	// Generate unique frame ID
	static std::atomic<unsigned int> frameCounter{0};
	unsigned int frameId = frameCounter++;

	const auto &buffers = request->buffers();
	if (buffers.empty()) {
		this->pipe()->completeRequest(request);
		return 0;
	}

	// Get the first buffer
	const Stream *stream = buffers.begin()->first;
	FrameBuffer *buffer = buffers.begin()->second;

	const StreamConfiguration &cfg = stream->configuration();
	unsigned int width = cfg.size.width;
	unsigned int height = cfg.size.height;

	LOG(SoftISPPipeline, Debug) << "Queue request: frame=" << frameId 
	                           << " size=" << width << "x" << height;

	// Create frame info to track this request (matches rkisp1/ipu3 pattern)
	SoftISPFrameInfo *info = frameInfo_.create(request, buffer, frameId);
	if (!info) {
		LOG(SoftISPPipeline, Error) << "Failed to create frame info";
		this->pipe()->completeRequest(request);
		return -ENOMEM;
	}

	// Get the FD from the buffer (SharedFD keeps it alive)
	const FrameBuffer::Plane &plane = buffer->planes()[0];
	SharedFD fd = plane.fd;

	// ============================================
	// CALL IPA (Async, Non-blocking)
	// IPA will process in background and signal via callback
	// ============================================

	// Stage 1: Process Stats (async)
	ipa_->processStats(frameId, 0, ControlList());
	
	// Stage 2: Process Frame (async)
	// IPA receives FD, maps internally if needed, processes, signals completion
	ipa_->processFrame(frameId, 0, fd, 0, width, height, ControlList());

	// Return immediately (don't wait for completion)
	// Completion will happen in metadataReady callback
	return 0;
}

int SoftISPCameraData::stop()
{
	running_ = false;
	return 0;
}

void SoftISPCameraData::run()
{
	running_ = true;
	while (running_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

// Callback: IPA signals stats processing complete (matches rkisp1/ipu3 pattern)
void SoftISPCameraData::metadataReady(unsigned int frame, const ControlList &metadata)
{
	LOG(SoftISPPipeline, Debug) << "metadataReady: frame=" << frame;

	SoftISPFrameInfo *info = frameInfo_.find(frame);
	if (!info) {
		LOG(SoftISPPipeline, Warning) << "metadataReady for unknown frame " << frame;
		return;
	}

	// Merge metadata into request
	info->request->_d()->metadata().merge(metadata);
	info->metadataProcessed = true;

	// Check if request can be completed
	tryCompleteRequest(info);
}

void SoftISPCameraData::tryCompleteRequest(SoftISPFrameInfo *info)
{
	// Check if all stages are complete
	if (!frameInfo_.tryCompleteRequest(info))
		return;

	// All stages complete, remove from pending and complete request
	frameInfo_.destroy(info->frame);
	this->pipe()->completeRequest(info->request);
	
	// FrameBuffer is now destroyed → FD closed → memfd memory freed automatically
	LOG(SoftISPPipeline, Debug) << "Request completed: frame=" << info->frame;
}

// Register pipeline handler
REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "softisp")

} /* namespace libcamera */
