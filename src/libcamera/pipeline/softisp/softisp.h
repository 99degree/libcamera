/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (dummy cameras)
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
#include <libcamera/ipa/softisp_ipa_proxy.h>

namespace libcamera {
class SoftISPConfiguration : public libcamera::CameraConfiguration {
public:
	SoftISPConfiguration();
	Status validate() override;
};

/* Forward declarations */
class PipelineHandlerSoftISP;
class VirtualCamera;

/*
 * SoftISPCameraData - Camera data structure for SoftISP dummy pipeline.
 * Must be defined BEFORE PipelineHandlerSoftISP.
 */
class SoftISPCameraData : public Camera::Private, public Thread
{
public:
	SoftISPCameraData(PipelineHandlerSoftISP *pipe);
	~SoftISPCameraData();

	int init();
	int loadIPA();

	void run() override;
	std::unique_ptr<Stream> dummyStream_;
	void processRequest(Request *request);
	FrameBuffer* getBufferFromId(uint32_t bufferId);

	struct StreamConfig {
		Stream *stream = nullptr;
		unsigned int seq = 0;
	};

	std::unique_ptr<ipa::soft::IPASoftIspInterface> ipa_;
	std::unique_ptr<VirtualCamera> virtualCamera_;
	std::vector<StreamConfig> streamConfigs_;
	bool running_ = false;
	Mutex mutex_;
	std::map<uint32_t, FrameBuffer*> bufferMap_; // Map bufferId -> FrameBuffer*
};

/*
 * Pipeline handler for SoftISP with dummy cameras.
 */
class PipelineHandlerSoftISP : public PipelineHandler
{
public:
	static bool created_;
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

	bool initFrameGenerator(Camera *camera);
	void bufferCompleted(FrameBuffer *buffer);

	DmaBufAllocator dmaBufAllocator_;
};

} /* namespace libcamera */
class SoftISPConfiguration : public libcamera::CameraConfiguration {
public:
	SoftISPConfiguration();
	Status validate() override;
};
