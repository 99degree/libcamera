/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (virtual and real cameras)
 */

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
#include "libcamera/internal/request.h"
#include "libcamera/internal/v4l2_videodevice.h"

namespace libcamera {

static std::map<uint32_t, int> g_bufferFdMap;

LOG_DEFINE_CATEGORY(SoftISPPipeline)

bool PipelineHandlerSoftISP::created_ = false;

/* -----------------------------------------------------------------------------
 * SoftISPConfiguration
 * ---------------------------------------------------------------------------*/

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

/* -----------------------------------------------------------------------------
 * SoftISPCameraData
 * ---------------------------------------------------------------------------*/

SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
    : Camera::Private(pipe), Thread("SoftISPCamera")
{
    virtualCamera_ = std::make_unique<VirtualCamera>();
}

SoftISPCameraData::~SoftISPCameraData()
{
    stop();
    wait();
}

int SoftISPCameraData::init()
{
    int ret = loadIPA();
    if (ret)
        return ret;

    if (isVirtualCamera) {
        ret = virtualCamera_->init(1920, 1080);
        if (ret) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize virtual camera";
            return ret;
        }
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

    LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded";
    return 0;
}

void SoftISPCameraData::run()
{
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

    storeBuffer(bufferId, buffer);

    const Stream *stream = buffers.begin()->first;
    auto streamConfig = stream->configuration();

    ControlList statsResults;

    // Call processStats
    ipa_->processStats(frameId, bufferId, statsResults);

    // Merge results into request metadata
    request->controls().merge(statsResults,
                             libcamera::ControlList::MergePolicy::OverwriteExisting);

    // Call processFrame with bufferFd
    const ControlList &results = request->controls();
    ipa_->processFrame(frameId, bufferId, plane.fd, 0,
                       streamConfig.size.width, streamConfig.size.height, results);

    munmap(bufferMem, plane.length);
    bufferMap_.erase(bufferId);

    request->controls().set(controls::SensorTimestamp,
                           static_cast<int64_t>(frameId * 33333));

    pipe()->completeRequest(request);
}

FrameBuffer* SoftISPCameraData::getBufferFromId(uint32_t bufferId)
{
    auto it = bufferMap_.find(bufferId);
    return (it != bufferMap_.end()) ? it->second : nullptr;
}

void SoftISPCameraData::storeBuffer(uint32_t bufferId, FrameBuffer *buffer)
{
    std::lock_guard<Mutex> lock(mutex_);
    bufferMap_[bufferId] = buffer;
}

/* -----------------------------------------------------------------------------
 * PipelineHandlerSoftISP
 * ---------------------------------------------------------------------------*/

PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager)
    : PipelineHandler(manager)
{
    LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler created";
}

PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
    LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler destroyed";
}

bool PipelineHandlerSoftISP::isV4LCamera(std::shared_ptr<MediaDevice> media)
{
    for (auto &entity : media->entities()) {
        if (entity.function() == MediaEntityFunction::CameraSensor ||
            entity.function() == MediaEntityFunction::V4L2VideoDevice) {
            return true;
        }
    }
    return false;
}

bool PipelineHandlerSoftISP::createRealCamera(std::shared_ptr<MediaDevice> media)
{
    LOG(SoftISPPipeline, Info) << "Creating real camera: " << media->name();

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

    if (!registerCamera(std::move(cameraData), *config)) {
        LOG(SoftISPPipeline, Warning) << "Failed to register real camera";
        return false;
    }

    LOG(SoftISPPipeline, Info) << "Real camera registered successfully";
    return true;
}

bool PipelineHandlerSoftISP::createVirtualCamera()
{
    LOG(SoftISPPipeline, Info) << "Creating virtual camera";

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

    if (!registerCamera(std::move(cameraData), *config)) {
        LOG(SoftISPPipeline, Error) << "Failed to register virtual camera";
        return false;
    }

    LOG(SoftISPPipeline, Info) << "Virtual camera registered successfully";
    return true;
}

bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
    LOG(SoftISPPipeline, Info) << "Matching SoftISP cameras";

    // Step 1: Try to find real V4L2 cameras
    std::vector<std::shared_ptr<MediaDevice>> realCameras;

    if (enumerator) {
        enumerator->enumerate();
        for (auto &device : enumerator->devices()) {
            if (isV4LCamera(device)) {
                realCameras.push_back(device);
                LOG(SoftISPPipeline, Info) << "Found real camera: " << device->name();
            }
        }
    }

    // Step 2: If real cameras found, create them
    for (auto &media : realCameras) {
        if (!createRealCamera(media)) {
            LOG(SoftISPPipeline, Warning) << "Failed to create real camera";
        }
    }

    // Step 3: If NO real cameras found, create a virtual camera
    if (realCameras.empty()) {
        LOG(SoftISPPipeline, Info) << "No real cameras found, creating virtual camera";
        return createVirtualCamera();
    }

    LOG(SoftISPPipeline, Info) << "Registered " << realCameras.size() << " real camera(s)";
    return !realCameras.empty();
}

std::unique_ptr<CameraConfiguration>
PipelineHandlerSoftISP::generateConfiguration(Camera *camera,
                                              Span<const StreamRole> roles)
{
    auto cameraData = cameraData(camera);

    auto config = std::make_unique<SoftISPConfiguration>();

    // Default configuration: 1920x1080 NV12
    StreamConfiguration cfg;
    cfg.size = Size(1920, 1080);
    cfg.pixelFormat = formats::NV12;
    cfg.bufferCount = 4;

    config->addConfiguration(cfg);

    LOG(SoftISPPipeline, Debug) << "Generated configuration: "
                                << cfg.size.toString() << " "
                                << cfg.pixelFormat.toString();

    return config;
}

int PipelineHandlerSoftISP::configure(Camera *camera, CameraConfiguration *config)
{
    auto cameraData = cameraData(camera);

    if (config->validate() == CameraConfiguration::Invalid)
        return -EINVAL;

    // Store stream configuration
    cameraData->streamConfigs_.resize(1);
    cameraData->streamConfigs_[0].stream = config->at(0).stream();

    LOG(SoftISPPipeline, Info) << "Configured camera: "
                               << config->at(0).size.toString();

    return 0;
}

int PipelineHandlerSoftISP::exportFrameBuffers(Camera *camera, Stream *stream,
                                               std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    auto cameraData = cameraData(camera);
    const auto &cfg = stream->configuration();

    Size size = cfg.size;
    PixelFormat format = cfg.pixelFormat;
    unsigned int bufferCount = cfg.bufferCount;

    // Calculate buffer size based on format
    size_t bufferSize = 0;
    if (format == formats::NV12) {
        bufferSize = size.width * size.height * 3 / 2;
    } else if (format == formats::RGB888) {
        bufferSize = size.width * size.height * 3;
    } else {
        bufferSize = size.width * size.height * 2; // Default
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

        auto buffer = std::make_unique<FrameBuffer>();
        buffer->planes().emplace_back(FrameBuffer::Plane{fd, bufferSize});
        fd.release(); // FrameBuffer takes ownership

        buffers->push_back(std::move(buffer));
    }

    return 0;
}

int PipelineHandlerSoftISP::start(Camera *camera, const ControlList *controls)
{
    auto cameraData = cameraData(camera);

    cameraData->running_ = true;
    cameraData->start();

    if (cameraData->isVirtualCamera) {
        cameraData->virtualCamera_->start();
    }

    LOG(SoftISPPipeline, Info) << "Camera started";
    return 0;
}

void PipelineHandlerSoftISP::stopDevice(Camera *camera)
{
    auto cameraData = cameraData(camera);

    cameraData->running_ = false;
    cameraData->stop();

    if (cameraData->isVirtualCamera) {
        cameraData->virtualCamera_->stop();
    }

    LOG(SoftISPPipeline, Info) << "Camera stopped";
}

int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request)
{
    auto cameraData = cameraData(camera);
    cameraData->processRequest(request);
    return 0;
}

} // namespace libcamera
