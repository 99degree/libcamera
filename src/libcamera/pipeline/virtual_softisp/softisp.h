/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP (virtual cameras)
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

class SoftISPCameraData;

/*
 * Pipeline handler for SoftISP with virtual cameras.
 *
 * This pipeline handler creates dummy/test cameras for testing
 * the SoftISP IPA module without requiring real hardware.
 */
class PipelineHandlerVirtualSoftISP : public PipelineHandler
{
public:
	PipelineHandlerVirtualSoftISP(CameraManager *manager);
	~PipelineHandlerVirtualSoftISP();

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

/*
 * SoftISPCameraData - Camera data structure for SoftISP virtual pipeline.
 */
class SoftISPCameraData : public Camera::Private, public Thread
{
public:
	SoftISPCameraData(PipelineHandlerVirtualSoftISP *pipe);
	~SoftISPCameraData();

	int init();
	int loadIPA();

	void run() override;
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

} /* namespace libcamera */
