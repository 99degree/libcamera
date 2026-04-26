/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (virtual and real cameras)
 */

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <libcamera/base/object.h>
#include <libcamera/base/thread.h>
#include <libcamera/base/mutex.h>
#include <libcamera/camera.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/pipeline_handler.h"
#include "libcamera/internal/dma_buf_allocator.h"
#include <libcamera/ipa/softisp_ipa_proxy.h>
#include "virtual_camera.h"
#include "placeholder_stream.h"

namespace libcamera {

// Forward declaration of the log category
extern const LogCategory &SoftISPPipeline;

class SoftISPConfiguration : public libcamera::CameraConfiguration {
public:
	SoftISPConfiguration();
	Status validate() override;
};

/* Forward declarations */
class PipelineHandlerSoftISP;

struct StreamConfig {
	Stream *stream = nullptr;
	unsigned int seq = 0;
};

struct SoftISPFrameInfo {
	unsigned int frame;
	Request *request;
	FrameBuffer *buffer;
	bool metadataReceived;
	bool frameReceived;

	SoftISPFrameInfo(Request *req, FrameBuffer *buf, unsigned int id)
		: frame(id), request(req), buffer(buf),
		  metadataReceived(false), frameReceived(false) {}
};

class SoftISPFrames {
public:
	SoftISPFrames() = default;
	SoftISPFrames(const SoftISPFrames &) = delete;
	SoftISPFrames &operator=(const SoftISPFrames &) = delete;
	SoftISPFrames(SoftISPFrames &&) = delete;
	SoftISPFrames &operator=(SoftISPFrames &&) = delete;

	SoftISPFrameInfo *create(Request *request, FrameBuffer *buffer, unsigned int frameId);
	SoftISPFrameInfo *find(unsigned int frame);
	int destroy(unsigned int frame);
	bool tryCompleteRequest(SoftISPFrameInfo *info);

private:
	std::map<unsigned int, std::unique_ptr<SoftISPFrameInfo>> frameInfo_;
};

class SoftISPCameraData : public Camera::Private {
public:
	SoftISPCameraData(PipelineHandlerSoftISP *pipe);
	~SoftISPCameraData();

	int init();
	int loadIPA();
	int configure(CameraConfiguration *config);
	int start(const ControlList *controls = nullptr);
	void stop();
	int queueRequest(Request *request);
    void frameDone(unsigned int frameId, unsigned int bufferId);
    int start();
	FrameBuffer* getBufferFromId(uint32_t bufferId);
	void storeBuffer(uint32_t bufferId, FrameBuffer *buffer);
	int exportFrameBuffers(const Stream *stream,
			       std::vector<std::unique_ptr<FrameBuffer>> *buffers);
	std::unique_ptr<CameraConfiguration> generateConfiguration(Span<const StreamRole> roles);

	SoftISPFrames frameInfo_;

	void metadataReady(unsigned int frame, const ControlList &metadata);
	void tryCompleteRequest(SoftISPFrameInfo *info);

	ipa::soft::IPASoftIspInterface *ipa() const { return ipa_.get(); }

	std::unique_ptr<ipa::soft::IPASoftIspInterface> ipa_;
	std::unique_ptr<VirtualCamera> frameGenerator_;
    libcamera::ipa::soft::IPASoftInterface *ipaInterface_ = nullptr;
    std::unique_ptr<IPAInterface> ipaInterfaceOwned_;
    // Track active requests: bufferId -> Request*
    std::map<unsigned int, Request*> activeRequests_;
    std::mutex requestsMutex_;
	std::vector<StreamConfig> streamConfigs_;
	bool running_ = false;
	Mutex mutex_;
	std::map<uint32_t, FrameBuffer*> bufferMap_;
	std::shared_ptr<MediaDevice> mediaDevice_;
	bool isVirtualCamera = true;

	// Store placeholder streams (like SimplePipeline)
	std::vector<PlaceholderStream> placeholderStreams_;
	PlaceholderStream *initialStream_ = nullptr;
};

class PipelineHandlerSoftISP : public PipelineHandler {
public:
	static bool s_virtualCameraRegistered;
	bool resetCreated_ = false;
	std::unique_ptr<VirtualCamera> frameGenerator_;
    libcamera::ipa::soft::IPASoftInterface *ipaInterface_ = nullptr;
    std::unique_ptr<IPAInterface> ipaInterfaceOwned_;
    // Track active requests: bufferId -> Request*
    std::map<unsigned int, Request*> activeRequests_;
    std::mutex requestsMutex_;

	PipelineHandlerSoftISP(CameraManager *manager);
	~PipelineHandlerSoftISP();

	std::unique_ptr<CameraConfiguration> generateConfiguration(
		Camera *camera, Span<const StreamRole> roles) override;
	int configure(Camera *camera, CameraConfiguration *config) override;
	int exportFrameBuffers(Camera *camera, Stream *stream,
			       std::vector<std::unique_ptr<FrameBuffer>> *buffers) override;
	int start(Camera *camera, const ControlList *controls) override;
	void stopDevice(Camera *camera) override;
	int queueRequestDevice(Camera *camera, Request *request) override;
	bool match(DeviceEnumerator *enumerator) override;

private:
	SoftISPCameraData *cameraData(Camera *camera)
	{
		return static_cast<SoftISPCameraData *>(camera->_d());
	}

	bool isV4LCamera(std::shared_ptr<MediaDevice> media);
	bool createRealCamera(std::shared_ptr<MediaDevice> media);
	bool createVirtualCamera();
};

} // namespace libcamera
