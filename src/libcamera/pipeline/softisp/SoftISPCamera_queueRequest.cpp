/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::queueRequest(Request *request)
{
    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }

    // Get the buffer ID from the request
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

    // Extract buffer ID (using fd as ID for simplicity)
    unsigned int bufferId = buffer->planes()[0].fd.get();

    // Track this request
    {
        std::lock_guard<std::mutex> lock(requestsMutex_);
        activeRequests_[bufferId] = request;
    }

    // Queue the request to VirtualCamera
    virtualCamera_->queueRequest(request);

    LOG(SoftISPPipeline, Debug) << "Request queued, bufferId=" << bufferId;
    return 0;
}

} /* namespace libcamera */
