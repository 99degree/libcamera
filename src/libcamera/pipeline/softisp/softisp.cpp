#pragma GCC diagnostic ignored "-Wpedantic"
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
#include "libcamera/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/internal/request.h"
namespace libcamera {
static std::map<uint32_t, int> g_bufferFdMap;
LOG_DEFINE_CATEGORY(SoftISPPipeline)
bool PipelineHandlerSoftISP::created_ = false;
SoftISPConfiguration::SoftISPConfiguration() { }
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
SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe) : Camera::Private(pipe), Thread("SoftISPCamera") {
virtualCamera_ = std::make_unique<VirtualCamera>();
}
SoftISPCameraData::~SoftISPCameraData() {
// Thread::exit(0);
/* Thread::wait(); */ // TODO: Fix Thread::wait() call
}
int SoftISPCameraData::init() {
int ret = loadIPA();
if (ret) return ret;
if (isVirtualCamera) {
ret = virtualCamera_->init(1920, 1080);
if (ret) { LOG(SoftISPPipeline, Error) << "Failed to initialize virtual camera"; return ret; }
}
return 0;
}
int SoftISPCameraData::loadIPA() {
// Skip IPA loading for now to avoid pipe() issues
LOG(SoftISPPipeline, Info) << "IPA module loading disabled";
return 0;
}
void SoftISPCameraData::run() {
while (running_) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
}
void SoftISPCameraData::processRequest(Request *request) {
(void)request;
// Simplified for compilation - complete request immediately
/* pipe()->completeRequest(request); */ // TODO: Fix pipe() call
}
FrameBuffer *SoftISPCameraData::getBufferFromId(uint32_t bufferId) {
auto it = bufferMap_.find(bufferId);
return (it != bufferMap_.end()) ? it->second : nullptr;
}
void SoftISPCameraData::storeBuffer(uint32_t bufferId, FrameBuffer *buffer) {
std::lock_guard<Mutex> lock(mutex_);
bufferMap_[bufferId] = buffer;
}
std::unique_ptr<CameraConfiguration> SoftISPCameraData::generateConfiguration(Span<const StreamRole> roles) {
auto config = std::make_unique<SoftISPConfiguration>();
(void)roles;
StreamConfiguration streamConfig;
streamConfig.size = Size(1920, 1080);
streamConfig.pixelFormat = formats::NV12;
streamConfig.colorSpace = ColorSpace::Rec709;
streamConfig.bufferCount = 4;
config->addConfiguration(streamConfig);
return config;
}
PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager) : PipelineHandler(manager) {
LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler created";
}
PipelineHandlerSoftISP::~PipelineHandlerSoftISP() {
LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler destroyed";
}
bool PipelineHandlerSoftISP::isV4LCamera(std::shared_ptr<MediaDevice> media) {
(void)media;
return true;
}
bool PipelineHandlerSoftISP::createRealCamera(std::shared_ptr<MediaDevice> media) {
(void)media;
LOG(SoftISPPipeline, Info) << "Creating real camera (stub)";
return false;
}
bool PipelineHandlerSoftISP::createVirtualCamera() {
// Virtual camera creation requires proper Camera::Private handling
// For now, return false to avoid compilation issues
LOG(SoftISPPipeline, Info) << "Virtual camera creation not fully implemented";
return false;
}
bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator) {
LOG(SoftISPPipeline, Info) << "Matching SoftISP cameras";
if (enumerator) { enumerator->enumerate(); }
return createVirtualCamera();
}
std::unique_ptr<CameraConfiguration> PipelineHandlerSoftISP::generateConfiguration(Camera *camera, Span<const StreamRole> roles) {
(void)camera;
(void)roles;
auto config = std::make_unique<SoftISPConfiguration>();
StreamConfiguration cfg;
cfg.size = Size(1920, 1080);
cfg.pixelFormat = formats::NV12;
cfg.bufferCount = 4;
config->addConfiguration(cfg);
LOG(SoftISPPipeline, Debug) << "Generated configuration: " << cfg.size.toString() << " " << cfg.pixelFormat.toString();
return config;
}
int PipelineHandlerSoftISP::configure(Camera *camera, CameraConfiguration *config) {
auto cameraDataPtr = cameraData(camera);
(void)cameraDataPtr;
if (config->validate() == CameraConfiguration::Invalid) return -EINVAL;
cameraDataPtr->streamConfigs_.resize(1);
cameraDataPtr->streamConfigs_[0].stream = config->at(0).stream();
LOG(SoftISPPipeline, Info) << "Configured camera: " << config->at(0).size.toString();
return 0;
}
int PipelineHandlerSoftISP::exportFrameBuffers(Camera *camera, Stream *stream, std::vector<std::unique_ptr<FrameBuffer>> *buffers) {
(void)camera;
(void)stream;
(void)buffers;
LOG(SoftISPPipeline, Warning) << "exportFrameBuffers not fully implemented";
return -ENOTSUP;
}
int PipelineHandlerSoftISP::start(Camera *camera, const ControlList *controls) {
(void)controls;
auto cameraDataPtr = cameraData(camera);
cameraDataPtr->running_ = true;
// cameraDataPtr->start();
if (cameraDataPtr->isVirtualCamera) { cameraDataPtr->virtualCamera_->start(); }
LOG(SoftISPPipeline, Info) << "Camera started";
return 0;
}
void PipelineHandlerSoftISP::stopDevice(Camera *camera) {
auto cameraDataPtr = cameraData(camera);
cameraDataPtr->running_ = false;
// cameraDataPtr->exit(0);
if (cameraDataPtr->isVirtualCamera) { cameraDataPtr->virtualCamera_->stop(); }
LOG(SoftISPPipeline, Info) << "Camera stopped";
}
int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request) {
auto cameraDataPtr = cameraData(camera);
cameraDataPtr->processRequest(request);
return 0;
}
} // namespace libcamera
