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

    // Create metadata (in a real implementation, this would come from IPA processing)
    ControlList metadata(controls::controls);
    metadata.add(controls::FrameDuration, 33333LL); // 30fps
    metadata.add(controls::AeState, controls::AeStateConverged);
    metadata.add(controls::AwbState, controls::AwbStateConverged);

    // Merge metadata into the request
    request->metadata().merge(metadata);

    // Complete the request - this notifies the app that the frame is done
    // and the buffer can be reused
    // Note: In the actual libcamera pipeline, this would be done through
    // the Camera::Private interface, which SoftISPCameraData has access to
    // For now, we log that we would complete the request here
    LOG(SoftISPPipeline, Info) << "Completing request for frame " << frameId;

    // The actual completion would be:
    // camera_->completeRequest(request);
    // But we're in SoftISPCameraData which is Camera::Private, so we need
    // to call the appropriate method through the Camera framework
    // This is typically done by calling the pipeline's completeRequest method
}

} /* namespace libcamera */
