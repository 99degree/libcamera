/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include "libcamera/internal/device_enumerator.h"
#include <algorithm>
#include <set>

namespace libcamera {

bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
	(void)enumerator;

	LOG(SoftISPPipeline, Info) << "match() called";
	
	// If we've already created the virtual camera, don't match again
	if (created_) {
		LOG(SoftISPPipeline, Info) << "Virtual camera already registered, skipping";
		return false;
	}
	
	created_ = true;
	LOG(SoftISPPipeline, Info) << "No real cameras found, registering SoftISP virtual camera";
	
	// Create camera data
	auto data = std::make_unique<SoftISPCameraData>(this);
	if (data->init() < 0) {
		LOG(SoftISPPipeline, Error) << "Failed to initialize camera data";
		return false;
	}
	
	// Create the camera with an empty streams set
	std::set<Stream *> streams;
	auto camera = Camera::create(std::move(data), "softisp_virtual", streams);
	if (!camera) {
		LOG(SoftISPPipeline, Error) << "Failed to create camera";
		return false;
	}
	
	// Register the camera so it persists
	registerCamera(std::move(camera));
	LOG(SoftISPPipeline, Info) << "Virtual camera registered: softisp_virtual";
	
	return true;
}

} /* namespace libcamera */
