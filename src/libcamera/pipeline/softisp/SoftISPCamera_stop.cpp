/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

void SoftISPCameraData::stop()
{
    LOG(SoftISPPipeline, Info) << "Stopping camera";
    
    // Stop the frame generator if it exists
    if (frameGenerator_) {
        frameGenerator_->stop();
    }
}

} /* namespace libcamera */
