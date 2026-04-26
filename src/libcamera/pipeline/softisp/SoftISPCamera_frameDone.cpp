/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftISPCameraData::frameDone(unsigned int frameId, unsigned int bufferId)
{
	LOG(SoftISPPipeline, Info) << "Frame done: frameId=" << frameId
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

	if (request) {
		pipe()->completeRequest(request);
	}
}
