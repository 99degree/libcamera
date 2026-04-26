/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftISPCameraData::frameDone(unsigned int frameId, unsigned int bufferId)
{
	LOG(SoftISPPipeline, Debug) << "Frame done: frameId=" << frameId
				    << ", bufferId=" << bufferId;

	Request *request = nullptr;

	{
		std::lock_guard<std::mutex> lock(requestsMutex_);
		auto it = activeRequests_.find(bufferId);
		if (it != activeRequests_.end()) {
			request = it->second;
			activeRequests_.erase(it);
		}
	}

	if (!request) {
		LOG(SoftISPPipeline, Warning) << "No active request for bufferId=" << bufferId;
		return;
	}

	/*
	 * Complete the buffer with matching fd.
	 */
	for (auto &[stream, buffer] : request->buffers()) {
		if (!buffer || buffer->planes().empty())
			continue;

		int actual = buffer->planes()[0].fd.get();
		if (actual == (int)bufferId) {
			pipe()->completeBuffer(request, buffer);
			break;
		}
	}

	pipe()->completeRequest(request);
}