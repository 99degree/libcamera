/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - Stub implementation without ONNX runtime.
 */
#include "softisp.h"
#include <libcamera/base/log.h>

namespace libcamera {
LOG_DEFINE_CATEGORY(SoftIsp)

namespace ipa::soft {

SoftIsp::SoftIsp() : impl_(std::make_unique<Impl>())
{
	LOG(SoftIsp, Info) << "SoftIsp stub created";
}

SoftIsp::~SoftIsp() = default;

int32_t SoftIsp::init(const IPASettings & /*settings*/,
		      const SharedFD & /*fdStats*/,
		      const SharedFD & /*fdParams*/,
		      const IPACameraSensorInfo &sensorInfo,
		      const ControlInfoMap & /*sensorControls*/,
		      ControlInfoMap * /*ipaControls*/,
		      bool * /*ccmEnabled*/)
{
	impl_->initialized = true;
	impl_->imageWidth = sensorInfo.activeArea.width;
	impl_->imageHeight = sensorInfo.activeArea.height;
	
	LOG(SoftIsp, Info) << "SoftISP initialized for " 
			   << impl_->imageWidth << "x" << impl_->imageHeight;
	return 0;
}

int32_t SoftIsp::start()
{
	LOG(SoftIsp, Info) << "SoftISP started";
	return 0;
}

void SoftIsp::stop()
{
	LOG(SoftIsp, Info) << "SoftISP stopped";
}

int32_t SoftIsp::configure(const IPAConfigInfo & /*configInfo*/)
{
	LOG(SoftIsp, Info) << "SoftISP configured";
	return 0;
}

void SoftIsp::queueRequest(const uint32_t frame, const ControlList & /*controls*/)
{
	LOG(SoftIsp, Debug) << "queueRequest: frame=" << frame;
}

void SoftIsp::computeParams(const uint32_t frame)
{
	LOG(SoftIsp, Debug) << "computeParams: frame=" << frame;
}

void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId,
			   ControlList & /*stats*/)
{
	LOG(SoftIsp, Debug) << "processStats: frame=" << frame << ", buffer=" << bufferId;
}


std::string SoftIsp::logPrefix() const
{
	return "SoftIsp";
}

} /* namespace soft */
} /* namespace libcamera */
