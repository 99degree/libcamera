/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::init()
{
    LOG(SoftISPPipeline, Info) << "Initializing SoftISPCameraData";

    isVirtualCamera = true;
    virtualCamera_ = std::make_unique<VirtualCamera>();
    int ret = virtualCamera_->init(1920, 1080);
    if (ret) {
        LOG(SoftISPPipeline, Error) << "Failed to initialize VirtualCamera";
        return ret;
    }

    // Set the frameDone callback to notify when a frame is processed
    virtualCamera_->setFrameDoneCallback([this](unsigned int frame, uint32_t bufferId) {
        this->frameDone(frame, bufferId);
    });

    LOG(SoftISPPipeline, Info) << "VirtualCamera initialized (waiting for start())";
    return 0;
}

} /* namespace libcamera */
