/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include "virtual_camera.h"

namespace libcamera {

bool PipelineHandlerSoftISP::createVirtualCamera()
{
	LOG(SoftISPPipeline, Info) << "Creating SoftISP virtual camera";
	
	if (!virtualCamera_) {
		virtualCamera_ = std::make_unique<VirtualCamera>();
		int ret = virtualCamera_->init(1920, 1080);
		if (ret) {
			LOG(SoftISPPipeline, Error) << "Failed to initialize virtual camera";
			return false;
		}
	}
	
	LOG(SoftISPPipeline, Info) << "SoftISP virtual camera initialized: 1920x1080";
	return true;
}

} /* namespace libcamera */
