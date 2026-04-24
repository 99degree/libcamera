/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - ONNX-based Image Processing Algorithm.
 * Skeleton implementation that includes method implementations.
 */
#include "softisp.h"
#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>
#include <cstdlib>

namespace libcamera {
LOG_DEFINE_CATEGORY(SoftIsp)

namespace ipa::soft {

SoftIsp::SoftIsp() : impl_(std::make_unique<Impl>())
{
	LOG(SoftIsp, Info) << "SoftIsp created";
}

SoftIsp::~SoftIsp() = default;

#include "init.cpp"
#include "start.cpp"
#include "stop.cpp"
#include "configure.cpp"
#include "queueRequest.cpp"
#include "computeParams.cpp"
#include "processStats.cpp"
#include "processFrame.cpp"
#include "logPrefix.cpp"

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */
