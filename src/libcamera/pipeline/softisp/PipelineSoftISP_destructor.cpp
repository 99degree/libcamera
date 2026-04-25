/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

PipelineHandlerSoftISP::~PipelineHandlerSoftISP() {
{
    LOG(SoftISPPipeline, Info) << "Destroying PipelineHandlerSoftISP";
    created_ = false;  // Reset so we can match again
}

} /* namespace libcamera */
