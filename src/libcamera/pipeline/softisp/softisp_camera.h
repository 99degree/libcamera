/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024 George Chan <gchan9527@gmail.com>
 *
 * SoftISPCameraData - Camera Object declaration
 */

#pragma once

#include <memory>
#include <vector>
#include <cstdint>

#include <libcamera/base/mutex.h>
#include <libcamera/base/thread.h>

#include "libcamera/internal/camera.h"

namespace libcamera {

class PipelineHandlerSoftISP;
class VirtualCamera;
class SoftISPConfiguration;

/**
 * \class SoftISPCameraData softisp_camera.h
 * rief The Camera Object that holds all camera-specific state and logic.
 */
class SoftISPCameraData : public Camera::Private, public Thread
{
public:
	SoftISPCameraData(PipelineHandlerSoftISP *pipe);
	~SoftISPCameraData();

	// Initialization
	int init();

	// Camera Object Interface
	std::unique_ptr<CameraConfiguration> generateConfiguration(Span<const StreamRole> roles);
	int configure(CameraConfiguration *config);
	int exportFrameBuffers(Stream *stream, std::vector<std::unique_ptr<FrameBuffer>> *buffers);
	int start(const ControlList *controls);
	void stop();
	int queueRequest(Request *request);

	int loadIPA();
	FrameBuffer *getBufferFromId(uint32_t bufferId);
	void storeBuffer(uint32_t bufferId, FrameBuffer *buffer);
	void processRequest(Request *request);
	// Get VirtualCamera
	VirtualCamera *virtualCamera() { return virtualCamera_.get(); }
	const VirtualCamera *virtualCamera() const { return virtualCamera_.get(); }

private:
	void run() override;

	std::unique_ptr<VirtualCamera> virtualCamera_;
	Mutex mutex_;
};

} // namespace libcamera
