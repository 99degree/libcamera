/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>
#include <cstdlib>

namespace libcamera {
LOG_DEFINE_CATEGORY(SoftIsp)

namespace ipa::soft {

SoftIsp::SoftIsp() : impl_(std::make_unique<Impl>())
{
}

SoftIsp::~SoftIsp() = default;

#include "SoftIsp_init.cpp"
#include "SoftIsp_start.cpp"
#include "SoftIsp_stop.cpp"
#include "SoftIsp_configure.cpp"
#include "SoftIsp_queueRequest.cpp"
#include "SoftIsp_computeParams.cpp"
#include "SoftIsp_processStats.cpp"
#include "SoftIsp_processFrame.cpp"
#include "SoftIsp_logPrefix.cpp"

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */
