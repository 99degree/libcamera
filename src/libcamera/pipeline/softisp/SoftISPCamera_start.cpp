/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::start([[maybe_unused]] const ControlList *controls)
{
	LOG(SoftISPPipeline, Info) << "Starting camera";
	if (!frameGenerator_) {
		LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
		return -EINVAL;
	}
	
	// IPA is loaded once in init(), here we just connect it on each start
	if (ipa_ && ipa_->isValid()) {
		LOG(SoftISPPipeline, Info) << "Connecting IPA to VirtualCamera";
		frameGenerator_->setIpaInterface(ipa());
	}

	running_ = true;
	return frameGenerator_->start();
}
