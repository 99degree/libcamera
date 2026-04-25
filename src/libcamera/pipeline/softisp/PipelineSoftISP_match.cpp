/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include "libcamera/internal/device_enumerator.h"
#include <algorithm>
#include <set>

namespace libcamera {

// Static storage for the virtual camera to persist across pipeline handler destruction
static std::shared_ptr<Camera> s_virtualCamera;
static bool s_cameraRegistered = false;

bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
	(void)enumerator;

	LOG(SoftISPPipeline, Info) << "=== match() called ===";
	LOG(SoftISPPipeline, Info) << "  created_=" << created_ << ", s_cameraRegistered=" << s_cameraRegistered;
	
	// If we've already created the virtual camera, don't match again
	if (created_) {
		LOG(SoftISPPipeline, Info) << "  Virtual camera already registered, returning false";
		return false;
	}
	
	created_ = true;
	LOG(SoftISPPipeline, Info) << "  Creating new virtual camera...";
	
	// Create camera data
	auto data = std::make_unique<SoftISPCameraData>(this);
	LOG(SoftISPPipeline, Info) << "  SoftISPCameraData created at " << data.get();
	
	if (data->init() < 0) {
		LOG(SoftISPPipeline, Error) << "  Failed to initialize camera data";
		return false;
	}
	
	LOG(SoftISPPipeline, Info) << "  Camera data initialized";
	
	// Create a basic stream configuration
	std::set<Stream *> streams;
	
	// Create the camera
	LOG(SoftISPPipeline, Info) << "  Calling Camera::create()...";
	auto camera = Camera::create(std::move(data), "softisp_virtual", streams);
	
	if (!camera) {
		LOG(SoftISPPipeline, Error) << "  Camera::create() returned nullptr!";
		return false;
	}
	
	LOG(SoftISPPipeline, Info) << "  Camera created at " << camera.get();
	
	// Store a static reference to prevent the camera from being destroyed
	s_virtualCamera = camera;
	s_cameraRegistered = true;
	LOG(SoftISPPipeline, Info) << "  Static reference stored, s_virtualCamera=" << s_virtualCamera.get();
	
	// Register the camera
	LOG(SoftISPPipeline, Info) << "  Calling registerCamera()...";
	registerCamera(std::move(camera));
	LOG(SoftISPPipeline, Info) << "  registerCamera() completed";
	
	LOG(SoftISPPipeline, Info) << "  s_virtualCamera after register: " << s_virtualCamera.get();
	LOG(SoftISPPipeline, Info) << "=== match() returning true ===";
	return true;
}

} /* namespace libcamera */
