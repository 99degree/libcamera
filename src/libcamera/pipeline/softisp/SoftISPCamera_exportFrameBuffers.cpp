/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::exportFrameBuffers([[maybe_unused]] const Stream *stream,
                                          std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    LOG(SoftISPPipeline, Info) << "Exporting frame buffers";

    if (!frameGenerator_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }

    // Get buffers from VirtualCamera
    auto &vcBuffers = frameGenerator_->getBuffers();
    
    if (vcBuffers.empty()) {
        LOG(SoftISPPipeline, Error) << "No buffers allocated";
        return -EINVAL;
    }

    // Export the buffers
    for (auto *buffer : vcBuffers) {
        buffers->push_back(std::unique_ptr<FrameBuffer>(buffer));
    }

    LOG(SoftISPPipeline, Info) << "Exported " << buffers->size() << " buffers";
    return 0;
}

} /* namespace libcamera */
