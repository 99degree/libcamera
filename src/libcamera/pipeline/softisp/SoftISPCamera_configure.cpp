/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::configure([[maybe_unused]] CameraConfiguration *config)
{
    LOG(SoftISPPipeline, Info) << "Configuring camera";
    
    if (!frameGenerator_) {
        LOG(SoftISPPipeline, Error) << "Frame generator not initialized";
        return -EINVAL;
    }
    
    // Extract resolution from config
    const auto &streamConfig = config->at(0);
    unsigned int width = streamConfig.size.width;
    unsigned int height = streamConfig.size.height;
    
    LOG(SoftISPPipeline, Info) << "Configuration: " << width << "x" << height;
    
    // Initialize frame generator with the requested resolution
    int ret = frameGenerator_->init(width, height);
    if (ret) {
        LOG(SoftISPPipeline, Error) << "Failed to initialize frame generator";
        return ret;
    }
    
    // Allocate buffers in frame generator
    ret = frameGenerator_->allocateBuffers(4);
    if (ret) {
        LOG(SoftISPPipeline, Error) << "Failed to allocate buffers";
        return ret;
    }
    
    LOG(SoftISPPipeline, Info) << "Configuration complete (frame generator manages buffers)";
    return 0;
}

} /* namespace libcamera */
