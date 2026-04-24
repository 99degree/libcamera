/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Copyright (C) 2024 Pipeline handler for SoftISP */
#include "softisp.h"
#include "virtual_camera.h"
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <queue>
#include <sys/mman.h>
#include <unistd.h>
#include <libcamera/base/log.h>
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

static std::map<uint32_t, int> g_bufferFdMap;
LOG_DEFINE_CATEGORY(SoftISPPipeline)
bool PipelineHandlerSoftISP::created_ = false;

SoftISPConfiguration::SoftISPConfiguration() {}

CameraConfiguration::Status SoftISPConfiguration::validate() {
    if (empty()) return Invalid;
    Status status = Valid;
    for (auto it = begin(); it != end(); ++it) {
        StreamConfiguration &cfg = *it;
        if (cfg.size.width == 0 || cfg.size.height == 0) return Invalid;
        if (cfg.pixelFormat == 0) return Invalid;
    }
    return status;
}

SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
    : Camera::Private(pipe), Thread("SoftISPCamera")
{
    virtualCamera_ = std::make_unique<VirtualCamera>();
}

SoftISPCameraData::~SoftISPCameraData() {
    Thread::exit(0);
    wait();
}

int SoftISPCameraData::init() {
    int ret = loadIPA();
    if (ret) return ret;
    if (isVirtualCamera) {
        ret = virtualCamera_->init(1920, 1080);
        if (ret) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize virtual camera";
            return ret;
        }
    }
    return 0;
}

int SoftISPCameraData::loadIPA() {
    ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(
        this->pipe(), 1, 1);
    if (!ipa_) {
        LOG(SoftISPPipeline, Info) << "IPA module not available";
        return 0;
    }
    LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded";
    return 0;
}

void SoftISPCameraData::run() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void SoftISPCameraData::processRequest(Request *request) {
    if (!ipa_) {
        this->pipe()->completeRequest(request);
        return;
    }

    static uint32_t frameCounter = 0;
    uint32_t frameId = frameCounter++;

    const auto &buffers = request->buffers();
    if (buffers.empty()) {
        this->pipe()->completeRequest(request);
        return;
    }

    FrameBuffer *buffer = buffers.begin()->second;
    if (!buffer || buffer->planes().empty()) {
        this->pipe()->completeRequest(request);
        return;
    }

    uint32_t bufferId = static_cast<uint32_t>(buffer->planes()[0].fd.get());
    const auto &plane = buffer->planes()[0];

    void *bufferMem = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE,
                           MAP_SHARED, plane.fd.get(), 0);
    if (bufferMem == MAP_FAILED) {
        this->pipe()->completeRequest(request);
        return;
    }

    const Stream *stream = buffers.begin()->first;
	auto streamConfig = stream->configuration();
	storeBuffer(bufferId, buffer);

	// Generate Bayer pattern for virtual camera
	if (isVirtualCamera && virtualCamera_) {
		virtualCamera_->queueBuffer(buffer);
	}

    ControlList statsResults;
    ipa_->processStats(frameId, bufferId, statsResults);

    request->controls().merge(statsResults,
                              libcamera::ControlList::MergePolicy::OverwriteExisting);

    const ControlList &results = request->controls();
    ipa_->processFrame(frameId, bufferId, plane.fd, 0,
                       streamConfig.size.width, streamConfig.size.height,
                       results);

    munmap(bufferMem, plane.length);
    bufferMap_.erase(bufferId);

    request->controls().set(controls::SensorTimestamp,
                            static_cast<int64_t>(frameId * 33333));
    this->pipe()->completeRequest(request);
}

FrameBuffer *SoftISPCameraData::getBufferFromId(uint32_t bufferId) {
    auto it = bufferMap_.find(bufferId);
    return (it != bufferMap_.end()) ? it->second : nullptr;
}

void SoftISPCameraData::storeBuffer(uint32_t bufferId, FrameBuffer *buffer) {
    std::lock_guard<Mutex> lock(mutex_);
    bufferMap_[bufferId] = buffer;
}

std::unique_ptr<CameraConfiguration>
SoftISPCameraData::generateConfiguration(Span<const StreamRole> roles) {
	LOG(SoftISPPipeline, Info) << "SoftISPCameraData::generateConfiguration called";
    LOG(SoftISPPipeline, Info) << "Roles size: " << roles.size() << ", role[0]: " << static_cast<int>(roles[0]);
    auto config = std::make_unique<SoftISPConfiguration>();

    StreamConfiguration streamConfig;
    streamConfig.size = Size(1920, 1080);
    streamConfig.pixelFormat = formats::SBGGR10; // Bayer RGGB 10-bit
    streamConfig.colorSpace = ColorSpace::Rec709;
    streamConfig.bufferCount = 4;
    config->addConfiguration(streamConfig);
	LOG(SoftISPPipeline, Info) << "Config created, size=" << config->size();
	auto validationResult = config->validate();
	LOG(SoftISPPipeline, Info) << "Validation result: " << validationResult;

	LOG(SoftISPPipeline, Info) << "Returning config with size=" << config->size();
    return config;
}

PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager)
    : PipelineHandler(manager)
{
    LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler created";
}

PipelineHandlerSoftISP::~PipelineHandlerSoftISP() {
	if (resetCreated_)
		created_ = false;
}

bool PipelineHandlerSoftISP::isV4LCamera(std::shared_ptr<MediaDevice> media) {
    (void)media;
    return true;
}

bool PipelineHandlerSoftISP::createRealCamera(std::shared_ptr<MediaDevice> media) {
    LOG(SoftISPPipeline, Info) << "Creating real camera: " << media->driver();

    auto cameraData = std::make_unique<SoftISPCameraData>(this);
    cameraData->mediaDevice_ = media;
    cameraData->isVirtualCamera = false;

    if (cameraData->init() < 0) {
        LOG(SoftISPPipeline, Warning) << "Failed to initialize real camera";
        return false;
    }

    std::vector<StreamRole> roles = { StreamRole::Viewfinder };
    auto config = cameraData->generateConfiguration(roles);
    if (!config || config->validate() == CameraConfiguration::Invalid) {
        LOG(SoftISPPipeline, Warning) << "Invalid configuration for real camera";
        return false;
    }

    std::shared_ptr<Camera> camera = Camera::create(
        std::unique_ptr<Camera::Private>(cameraData.release()),
        media->driver(), std::set<Stream *>());
    if (!camera) {
        LOG(SoftISPPipeline, Warning) << "Failed to create Camera object";
        return false;
    }

    registerCamera(std::move(camera));
    LOG(SoftISPPipeline, Info) << "Real camera registered successfully";
    return true;
}

bool PipelineHandlerSoftISP::createVirtualCamera() {
    LOG(SoftISPPipeline, Info) << "createVirtualCamera() called";
    auto cameraData = std::make_unique<SoftISPCameraData>(this);
    cameraData->isVirtualCamera = true;
    if (cameraData->init() < 0) {
        LOG(SoftISPPipeline, Error) << "Failed to initialize virtual camera";
        return false;
    }
    std::vector<StreamRole> roles = { StreamRole::Viewfinder };
    auto config = cameraData->generateConfiguration(roles);
    if (!config || config->validate() == CameraConfiguration::Invalid) {
        LOG(SoftISPPipeline, Error) << "Invalid configuration for virtual camera";
        return false;
    }
    // Create Camera object - pass cameraData directly (it inherits from Camera::Private)
    std::shared_ptr<Camera> camera = Camera::create(
        std::move(cameraData),
        "softisp_virtual", std::set<Stream *>());
    if (!camera) {
        LOG(SoftISPPipeline, Error) << "Failed to create Camera object";
        return false;
    }
    registerCamera(std::move(camera));
    LOG(SoftISPPipeline, Info) << "Virtual camera registered successfully";
    return true;
}

bool PipelineHandlerSoftISP::match([[maybe_unused]] DeviceEnumerator *enumerator) {
	if (created_)
		return false;
	created_ = true;
	LOG(SoftISPPipeline, Info) << "Creating SoftISP virtual camera";
	if (!createVirtualCamera()) {
		created_ = false;
		return false;
	}
	resetCreated_ = true;
	return true;
}

std::unique_ptr<CameraConfiguration> PipelineHandlerSoftISP::generateConfiguration(Camera *camera,
	Span<const StreamRole> roles) {
	// Division of Duty: PipelineHandler is the Dispatcher, SoftISPCameraData is the Worker
	SoftISPCameraData *cameraDataPtr = cameraData(camera);
	if (!cameraDataPtr) {
		LOG(SoftISPPipeline, Error) << "Failed to get camera data for generateConfiguration";
		return nullptr;
	}
	LOG(SoftISPPipeline, Info) << "PipelineHandlerSoftISP::generateConfiguration called (Dispatcher)";
	LOG(SoftISPPipeline, Info) << "Roles size: " << roles.size() << ", role[0]: " << static_cast<int>(roles[0]);
	// Delegate to the Camera Data (Worker) which holds the VirtualCamera state
	return cameraDataPtr->generateConfiguration(roles);
}


int PipelineHandlerSoftISP::configure(Camera *camera, CameraConfiguration *config) {
    auto cameraDataPtr = cameraData(camera);

    if (config->validate() == CameraConfiguration::Invalid)
        return -EINVAL;

    cameraDataPtr->streamConfigs_.resize(1);
    cameraDataPtr->streamConfigs_[0].stream = config->at(0).stream();

    LOG(SoftISPPipeline, Info) << "Configured camera: "
                               << config->at(0).size.toString();

    return 0;
}

int PipelineHandlerSoftISP::exportFrameBuffers(Camera *camera, Stream *stream,
                                               std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    auto cameraDataPtr = cameraData(camera);
    (void)cameraDataPtr;

    const auto &cfg = stream->configuration();
    Size size = cfg.size;
    PixelFormat format = cfg.pixelFormat;
    unsigned int bufferCount = cfg.bufferCount;

    size_t bufferSize = 0;
    if (format == formats::NV12) {
        bufferSize = size.width * size.height * 3 / 2;
    } else if (format == formats::RGB888) {
        bufferSize = size.width * size.height * 3;
    } else {
        bufferSize = size.width * size.height * 2;
    }

    LOG(SoftISPPipeline, Debug) << "Exporting " << bufferCount
                                << " buffers of size " << bufferSize;

    for (unsigned int i = 0; i < bufferCount; ++i) {
        int fd = memfd_create("softisp_buffer", MFD_CLOEXEC);
        if (fd < 0)
            return -errno;

        if (ftruncate(fd, bufferSize) < 0) {
            close(fd);
            return -errno;
        }

        std::vector<FrameBuffer::Plane> planes;
        FrameBuffer::Plane plane;
        plane.fd = SharedFD(fd);
        plane.length = static_cast<unsigned int>(bufferSize);
        planes.push_back(plane);
        auto buffer = std::make_unique<FrameBuffer>(planes);
        // fd is now owned by SharedFD, no need to close it
        buffers->push_back(std::move(buffer));
    }

    return 0;
}

int PipelineHandlerSoftISP::start(Camera *camera, const ControlList *controls) {
    auto cameraDataPtr = cameraData(camera);
    (void)controls;

    cameraDataPtr->running_ = true;
    cameraDataPtr->start();

    if (cameraDataPtr->isVirtualCamera) {
        cameraDataPtr->virtualCamera_->start();
    }

    LOG(SoftISPPipeline, Info) << "Camera started";
    return 0;
}

void PipelineHandlerSoftISP::stopDevice(Camera *camera) {
    auto cameraDataPtr = cameraData(camera);

    cameraDataPtr->running_ = false;
    cameraDataPtr->exit(0);

    if (cameraDataPtr->isVirtualCamera) {
        cameraDataPtr->virtualCamera_->Thread::exit(0);
    }

    LOG(SoftISPPipeline, Info) << "Camera stopped";
}

int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request) {
    auto cameraDataPtr = cameraData(camera);
    cameraDataPtr->processRequest(request);
    return 0;
}


REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "SoftISP")
} /* namespace libcamera */
