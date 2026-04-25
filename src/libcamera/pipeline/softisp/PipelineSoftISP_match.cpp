/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include "placeholder_stream.h"
#include <libcamera/formats.h>
#include "libcamera/internal/device_enumerator.h"
#include <algorithm>
#include <set>

namespace libcamera {

static std::shared_ptr<Camera> s_virtualCamera;
static bool s_cameraRegistered = false;

bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
	(void)enumerator;

	std::cerr << "DEBUG match() called" << std::endl;
	
	if (created_) {
		std::cerr << "DEBUG match(): already created, returning false" << std::endl;
		return false;
	}
	
	created_ = true;
	std::cerr << "DEBUG match(): creating camera data" << std::endl;
	
	auto data = std::make_unique<SoftISPCameraData>(this);
	std::cerr << "DEBUG match(): data=" << data.get() << std::endl;
	
	if (data->init() < 0) {
		std::cerr << "DEBUG match(): init failed" << std::endl;
		return false;
	}
	
	// Create a placeholder stream and store it in camera data
	data->initialStream_ = new PlaceholderStream();
	
	std::set<Stream *> streams;
	streams.insert(data->initialStream_);
	
	std::cerr << "DEBUG match(): calling Camera::create() with " << streams.size() << " stream(s)" << std::endl;
	std::cerr << "DEBUG match(): stream address: " << data->initialStream_ << std::endl;
	
	auto camera = Camera::create(std::move(data), "softisp_virtual", streams);
	
	if (!camera) {
		std::cerr << "DEBUG match(): Camera::create() returned nullptr" << std::endl;
		return false;
	}
	
	std::cerr << "DEBUG match(): camera=" << camera.get() << std::endl;
	
	s_virtualCamera = camera;
	s_cameraRegistered = true;
	
	std::cerr << "DEBUG match(): calling registerCamera()" << std::endl;
	registerCamera(std::move(camera));
	
	std::cerr << "DEBUG match(): returning true" << std::endl;
	return true;
}

} /* namespace libcamera */
