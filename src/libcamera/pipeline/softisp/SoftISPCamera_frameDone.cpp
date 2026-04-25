/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

void SoftISPCameraData::frameDone(unsigned int frameId, unsigned int bufferId)
{
    LOG(SoftISPPipeline, Info) << "Frame done: frameId=" << frameId << ", bufferId=" << bufferId;
    
    // Find the buffer in our map
    auto it = bufferMap_.find(bufferId);
    if (it == bufferMap_.end()) {
        LOG(SoftISPPipeline, Warning) << "Buffer not found for ID " << bufferId;
        return;
    }
    
    FrameBuffer *buffer = it->second;
    if (!buffer) {
        LOG(SoftISPPipeline, Warning) << "Null buffer for ID " << bufferId;
        return;
    }
    
    // In a real implementation, we would:
    // 1. Track which request used this buffer
    // 2. Merge any metadata from the IPA processing
    // 3. Call camera_->completeRequest(request)
    //
    // For the virtual camera, we'll simulate this by:
    // - Creating a minimal metadata set
    // - Finding the request that used this buffer (would need a request map)
    // - Completing the request
    
    // For now, just log - the full integration requires:
    // - Tracking request -> buffer mapping
    // - Access to camera_->completeRequest()
    // - Proper metadata merging
    
    LOG(SoftISPPipeline, Debug) << "Frame " << frameId << " processing complete";
}

} /* namespace libcamera */
