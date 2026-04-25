/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::start()
{
    LOG(SoftISPPipeline, Info) << "Starting camera";

    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }

    // Set up the frameDone callback
    virtualCamera_->setFrameDoneCallback([this](unsigned int frameId, unsigned int bufferId) {
        this->frameDone(frameId, bufferId);
    });

    // Start the VirtualCamera thread
    int ret = virtualCamera_->start();
    if (ret) {
        LOG(SoftISPPipeline, Error) << "Failed to start VirtualCamera";
        return ret;
    }

    LOG(SoftISPPipeline, Info) << "Camera started";
    return 0;
}

} /* namespace libcamera */
