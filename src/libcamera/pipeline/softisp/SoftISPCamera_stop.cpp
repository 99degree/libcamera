/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

void SoftISPCameraData::stop()
{
    LOG(SoftISPPipeline, Info) << "Stopping camera (delegating to VirtualCamera)";

    if (virtualCamera_) {
        virtualCamera_->stop();
    }

    LOG(SoftISPPipeline, Info) << "Camera stopped";
}

} /* namespace libcamera */
