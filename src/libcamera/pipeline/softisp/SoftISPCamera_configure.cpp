/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::configure([[maybe_unused]] CameraConfiguration *config)
{
    LOG(SoftISPPipeline, Info) << "Configuring camera";
    
    // Initialize the frame generator if needed
    if (frameGenerator_) {
        // Frame generator is already initialized
    }
    
    return 0;
}

} /* namespace libcamera */
