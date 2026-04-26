/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include "placeholder_stream.h"
#include <libcamera/formats.h>
#include <algorithm>
#include <set>

// Static camera that persists across pipeline handler lifetimes
static std::shared_ptr<Camera> s_persistentCamera;

bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
	(void)enumerator;
	LOG(SoftISPPipeline, Info) << "match() called";

	// If we already have a persistent camera, just return true
	if (s_persistentCamera) {
		LOG(SoftISPPipeline, Info) << "using existing persistent camera";
		registerCamera(s_persistentCamera);
		return true;
	}

	LOG(SoftISPPipeline, Info) << "creating new camera data";
	auto data = std::make_unique<SoftISPCameraData>(this);

	if (data->init() < 0) {
		LOG(SoftISPPipeline, Error) << "init failed";
		return false;
	}

	// Create a placeholder stream
	data->initialStream_ = new PlaceholderStream();
	std::set<Stream *> streams;
	streams.insert(data->initialStream_);

	auto camera = Camera::create(std::move(data), "softisp_virtual",
	                             streams);
	if (!camera) {
		LOG(SoftISPPipeline, Error) << "Camera::create() returned nullptr";
		return false;
	}

	// Store the camera statically so it persists
	s_persistentCamera = camera;

	registerCamera(std::move(camera));
	LOG(SoftISPPipeline, Info) << "match() returning true";

	return true;
}
