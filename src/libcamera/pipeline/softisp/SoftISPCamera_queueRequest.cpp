/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::queueRequest(Request *request)
{
    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }

    // Delegate request processing to VirtualCamera
    virtualCamera_->queueRequest(request);

    LOG(SoftISPPipeline, Debug) << "Request queued to VirtualCamera";
    return 0;
}

} /* namespace libcamera */
