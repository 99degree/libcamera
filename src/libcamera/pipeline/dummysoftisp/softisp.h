/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (dummy cameras)
 */
#pragma once

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
#include <libcamera/ipa/soft_ipa_interface.h>
#include <libcamera/ipa/soft_ipa_proxy.h>

namespace libcamera {

/* Forward declarations */
class PipelineHandlerDummysoftisp; // Forward declare before DummySoftISPCameraData uses it

/*
 * DummySoftISPCameraData - Camera data structure for SoftISP dummy pipeline.
 * Must be defined BEFORE PipelineHandlerDummysoftisp.
 */
class DummySoftISPCameraData : public Camera::Private, public Thread
{
public:
	DummySoftISPCameraData(PipelineHandlerDummysoftisp *pipe);
	~DummySoftISPCameraData();

	int init();
	int loadIPA();

	void run() override;
	std::unique_ptr<Stream> dummyStream_;
	void processRequest(Request *request);

	struct StreamConfig {
		Stream *stream = nullptr;
		unsigned int seq = 0;
	};

	std::unique_ptr<ipa::soft::IPAProxySoft> ipa_;
	std::vector<StreamConfig> streamConfigs_;
	bool running_ = false;
	Mutex mutex_;
};

/*
 * Pipeline handler for SoftISP with dummy cameras.
 */
class PipelineHandlerDummysoftisp : public PipelineHandler
{
public:
	static bool created_;
	PipelineHandlerDummysoftisp(CameraManager *manager);
	~PipelineHandlerDummysoftisp();

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
	DummySoftISPCameraData *cameraData(Camera *camera)
	{
		return static_cast<DummySoftISPCameraData *>(camera->_d());
	}

	bool initFrameGenerator(Camera *camera);
	void bufferCompleted(FrameBuffer *buffer);

	DmaBufAllocator dmaBufAllocator_;
};

} /* namespace libcamera */
