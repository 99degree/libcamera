/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::start()
{
    LOG(SoftISPPipeline, Info) << "Starting camera (delegating to VirtualCamera)";

    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }

    // Set up the frameDone callback
    virtualCamera_->setFrameDoneCallback([this](Request *request, ControlList &metadata) {
        // This is called when VirtualCamera finishes processing a frame
        // We need to complete the request in the Camera framework
        completeRequest(request, metadata);
    });

    // Start the VirtualCamera thread
    int ret = virtualCamera_->start();
    if (ret) {
        LOG(SoftISPPipeline, Error) << "Failed to start VirtualCamera";
        return ret;
    }

    LOG(SoftISPPipeline, Info) << "Camera started (VirtualCamera running)";
    return 0;
}

} /* namespace libcamera */
