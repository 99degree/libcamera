/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::init()
{
	LOG(SoftISPPipeline, Info) << "Initializing SoftISPCameraData";

	// Create virtual camera if it doesn't exist
	if (!frameGenerator_) {
		frameGenerator_ = std::make_unique<VirtualCamera>();

		// Initialize the virtual camera
		if (frameGenerator_->init(1920, 1080) < 0) {
			LOG(SoftISPPipeline, Error) << "Failed to initialize VirtualCamera";
			return -EINVAL;
		}

		// Set callback for frame completion from ISP processing
		frameGenerator_->setFrameDoneCallback(
		    [this](unsigned int frameId, unsigned int bufferId) {
		        LOG(SoftISPPipeline, Debug)
		            << "frameDone callback: frame=" << frameId
		            << ", buffer=" << bufferId;
		        frameDone(frameId, bufferId);
		    });
	}

	// Connect IPA interface if available (after IPA is loaded)
	if (ipa_ && ipa_->isValid()) {
		LOG(SoftISPPipeline, Info) << "Connecting IPA interface to virtual camera";
		frameGenerator_->setIpaInterface(ipa());
	}

	isVirtualCamera = true;
	return 0;
}