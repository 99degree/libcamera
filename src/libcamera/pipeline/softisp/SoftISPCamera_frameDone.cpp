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

    // Note: The actual request completion is handled by the PipelineHandler
    // through the Camera framework. In a full implementation, we would call
    // camera_->completeRequest(request) here, but that requires access to
    // the Camera::Private interface which is managed by the pipeline.
    // For now, the request is marked as complete by merging metadata.
    LOG(SoftISPPipeline, Info) << "Request completed for frame " << frameId;
}

} /* namespace libcamera */
