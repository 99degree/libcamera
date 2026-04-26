/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::init()
{
	LOG(SoftISPPipeline, Info) << "Initializing SoftISPCameraData";
	
	// Create virtual camera if it doesn't exist
	if (!frameGenerator_) {
		frameGenerator_ = std::make_unique<VirtualCamera>();
		
		// Initialize the virtual camera
		if (frameGenerator_->init(1920, 1080) < 0) {  // Default resolution
			LOG(SoftISPPipeline, Error) << "Failed to initialize VirtualCamera";
			return -EINVAL;
		}
		
		// Set callback for frame completion
		frameGenerator_->setFrameDoneCallback(
		    [this](unsigned int frameId, unsigned int bufferId) {
		        frameDone(frameId, bufferId);
		    });
	}
	
	isVirtualCamera = true;
	return 0;
}
