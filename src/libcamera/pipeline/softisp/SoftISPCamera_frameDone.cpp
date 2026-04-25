/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

void SoftISPCameraData::frameDone(unsigned int frameId, unsigned int bufferId)
{
    LOG(SoftISPPipeline, Info) << "Frame done: frameId=" << frameId << ", bufferId=" << bufferId;

    // Find the request associated with this buffer
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
        LOG(SoftISPPipeline, Warning) << "No request found for bufferId=" << bufferId;
        return;
    }

    // In a real implementation, we would:
    // 1. Get metadata from IPA processing
    // 2. Merge it into the request
    // 3. Complete the request
    
    // For now, just log
    LOG(SoftISPPipeline, Info) << "Frame " << frameId << " processing complete";
}

} /* namespace libcamera */
