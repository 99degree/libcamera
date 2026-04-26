/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

#include <libcamera/formats.h>

int SoftISPCameraData::configure([[maybe_unused]] CameraConfiguration *config)
{
	LOG(SoftISPPipeline, Info) << "Configuring camera";
	if (!frameGenerator_) {
		LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
		return -EINVAL;
	}

	auto *softConfig = static_cast<SoftISPConfiguration *>(config);

	auto status = softConfig->validate();
	if (status == CameraConfiguration::Invalid) {
		LOG(SoftISPPipeline, Error) << "Invalid camera configuration";
		return -EINVAL;
	}

	if (status == CameraConfiguration::Adjusted)
		LOG(SoftISPPipeline, Info) << "Configuration adjusted";

	/* Assign our stream and allocate buffers */
	for (auto &cfg : *config) {
		if (!initialStream_) {
			LOG(SoftISPPipeline, Error) << "No initial stream available";
			return -EINVAL;
		}
		cfg.setStream(initialStream_);
	}

	/* Allocate buffers based on the configured buffer count */
	unsigned int bufferCount = 4;
	if (!config->empty())
		bufferCount = config->at(0).bufferCount;

	frameGenerator_->allocateBuffers(bufferCount);

	return 0;
}