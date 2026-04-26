/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::exportFrameBuffers(
	[[maybe_unused]] const Stream *stream,
	std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	LOG(SoftISPPipeline, Info) << "Exporting frame buffers";
	if (!frameGenerator_) {
		LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
		return -EINVAL;
	}

	auto &genBuffers = frameGenerator_->getBuffers();
	for (auto *buffer : genBuffers) {
		buffers->push_back(
			std::unique_ptr<FrameBuffer>(buffer));
	}

	return 0;
}
