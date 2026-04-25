/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

void SoftISPCameraData::processRequest(Request *request)
{
    LOG(SoftISPPipeline, Info) << "Processing request";
    
    if (!request) {
        LOG(SoftISPPipeline, Error) << "Null request";
        return;
    }
    
    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return;
    }
    
    // Get the buffer map from the request
    const Request::BufferMap &bufferMap = request->buffers();
    if (bufferMap.empty()) {
        LOG(SoftISPPipeline, Error) << "No buffers in request";
        return;
    }
    
    // Get the first buffer (we only support one stream for now)
    FrameBuffer *buffer = nullptr;
    for (const auto &pair : bufferMap) {
        buffer = pair.second;
        break;
    }
    
    if (!buffer) {
        LOG(SoftISPPipeline, Error) << "Null buffer in request";
        return;
    }
    
    LOG(SoftISPPipeline, Debug) << "Processing buffer: " << buffer;
    
    // Queue the buffer to VirtualCamera for processing
    virtualCamera_->queueBuffer(request);
    
    LOG(SoftISPPipeline, Debug) << "Processed frame queued to VirtualCamera";
    LOG(SoftISPPipeline, Info) << "Request processing complete (virtual mode)";
}

} /* namespace libcamera */
