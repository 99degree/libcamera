/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::loadIPA()
{
    LOG(SoftISPPipeline, Info) << "IPA loading not available in this build";
    LOG(SoftISPPipeline, Info) << "VirtualCamera will generate frames directly (no IPA processing)";
    return 0;
}

} /* namespace libcamera */
