/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include "placeholder_stream.h"
#include <libcamera/formats.h>
#include <algorithm>
#include <set>

bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
	(void)enumerator;
	LOG(SoftISPPipeline, Info) << "match() called";

	// Only create the virtual camera once. Return false on subsequent
	// calls so libcamera stops probing.
	if (created_)
		return false;

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

	registerCamera(std::move(camera));
	created_ = true;
	LOG(SoftISPPipeline, Info) << "match() returning true";

	return true;
}