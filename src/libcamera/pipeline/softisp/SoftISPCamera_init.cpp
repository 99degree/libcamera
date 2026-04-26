/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::init()
{
	LOG(SoftISPPipeline, Info) << "Initializing SoftISPCameraData";

	if (!frameGenerator_) {
		frameGenerator_ = std::make_unique<VirtualCamera>();
		if (frameGenerator_->init(1920, 1080) < 0) {
			LOG(SoftISPPipeline, Error) << "Failed to initialize VirtualCamera";
			return -EINVAL;
		}

		frameGenerator_->setFrameDoneCallback(
		    [this](unsigned int frameId, unsigned int bufferId) {
		        frameDone(frameId, bufferId);
		    });
	}

	/* Load IPA once when camera data is created (persists across pipeline instances) */
	loadIPA();

	isVirtualCamera = true;
	return 0;
}