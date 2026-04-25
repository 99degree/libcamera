/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::loadIPA()
{
    LOG(SoftISPPipeline, Info) << "Loading IPA module";

    if (isVirtualCamera) {
        LOG(SoftISPPipeline, Info) << "Virtual camera mode - IPA loading skipped";
        return 0;
    }

    // In a real implementation, load the IPA module and get the interface
    // For now, we'll set a placeholder
    // ipaInterface_ = ...;

    if (virtualCamera_ && ipaInterface_) {
        virtualCamera_->setIPAInterface(ipaInterface_);
        LOG(SoftISPPipeline, Info) << "IPA interface connected to VirtualCamera";
    }

    return 0;
}

} /* namespace libcamera */
