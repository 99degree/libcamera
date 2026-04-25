/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::queueRequest(Request *request)
{
    LOG(SoftISPPipeline, Info) << "Queueing request";
    
    if (!request) {
        LOG(SoftISPPipeline, Error) << "Null request";
        return -EINVAL;
    }
    
    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }
    
    // Get the frame number from the request
    unsigned int frame = request->sequence();
    
    // Get the first buffer from the request
    const Request::BufferMap &bufferMap = request->buffers();
    if (bufferMap.empty()) {
        LOG(SoftISPPipeline, Error) << "No buffers in request";
        return -EINVAL;
    }
    
    FrameBuffer *buffer = bufferMap.begin()->second;
    if (!buffer) {
        LOG(SoftISPPipeline, Error) << "Null buffer in request";
        return -EINVAL;
    }
    
    // Queue the buffer to VirtualCamera for processing (with frame number)
    virtualCamera_->queueBuffer(buffer, frame);
    
    LOG(SoftISPPipeline, Debug) << "Frame " << frame << " queued to VirtualCamera";
    return 0;
}

} /* namespace libcamera */
