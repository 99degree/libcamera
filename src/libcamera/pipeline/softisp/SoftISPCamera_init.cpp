/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::init()
{
    LOG(SoftISPPipeline, Info) << "Initializing SoftISPCameraData";

    isVirtualCamera = true;
    
    // Create a new independent VirtualCamera instance (inherits from Thread)
    frameGenerator_ = std::make_unique<VirtualCamera>();
    
    if (!frameGenerator_) {
        LOG(SoftISPPipeline, Error) << "Failed to create VirtualCamera instance";
        return -ENOMEM;
    }

    LOG(SoftISPPipeline, Info) << "VirtualCamera instance created (Thread-based, independent)";
    return 0;
}

} /* namespace libcamera */
