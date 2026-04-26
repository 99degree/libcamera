/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::queueRequest(Request *request)
{
	if (!frameGenerator_) {
		LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
		return -EINVAL;
	}

	const Request::BufferMap &bufferMap = request->buffers();
	if (bufferMap.empty()) {
		LOG(SoftISPPipeline, Error) << "No buffers in request";
		return -EINVAL;
	}

	FrameBuffer *buffer = bufferMap.begin()->second;
	if (!buffer || buffer->planes().empty()) {
		LOG(SoftISPPipeline, Error) << "Invalid buffer in request";
		return -EINVAL;
	}

	unsigned int bufferId = buffer->planes()[0].fd.get();

	{
		std::lock_guard<std::mutex> lock(requestsMutex_);
		activeRequests_[bufferId] = request;
	}

	frameGenerator_->queueRequest(request);
	return 0;
}
