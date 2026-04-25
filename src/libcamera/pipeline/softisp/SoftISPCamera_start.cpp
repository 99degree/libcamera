/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::start([[maybe_unused]] const ControlList *controls)
{
    LOG(SoftISPPipeline, Info) << "Starting camera";
    
    // Start the frame generator if it exists
    if (frameGenerator_) {
        return frameGenerator_->start();
    }
    
    return 0;
}

} /* namespace libcamera */
