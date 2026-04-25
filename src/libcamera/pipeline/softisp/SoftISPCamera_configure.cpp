/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::configure(Camera *camera, CameraConfiguration *config)
{
    LOG(SoftISPPipeline, Info) << "Configuring camera (delegating to VirtualCamera)";

    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }

    // Extract resolution from config
    const auto &streamConfig = config->at(0);
    unsigned int width = streamConfig.size.width;
    unsigned int height = streamConfig.size.height;

    // Initialize VirtualCamera with the requested resolution
    int ret = virtualCamera_->init(width, height);
    if (ret) {
        LOG(SoftISPPipeline, Error) << "Failed to initialize VirtualCamera";
        return ret;
    }

    // Allocate buffers in VirtualCamera
    ret = virtualCamera_->allocateBuffers(4);
    if (ret) {
        LOG(SoftISPPipeline, Error) << "Failed to allocate buffers";
        return ret;
    }

    LOG(SoftISPPipeline, Info) << "Configuration complete (VirtualCamera manages buffers)";
    return 0;
}

} /* namespace libcamera */
