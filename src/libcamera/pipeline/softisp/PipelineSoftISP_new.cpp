/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include "virtual_camera.h"

bool PipelineHandlerSoftISP::createVirtualCamera()
{
	LOG(SoftISPPipeline, Info) << "Creating SoftISP virtual camera";
	
	// Create virtual camera if it doesn't exist
	if (!frameGenerator_) {
		frameGenerator_ = std::make_unique<VirtualCamera>();
		
		// Initialize the virtual camera
		frameGenerator_->init(1920, 1080);  // Default resolution
		
		// Connect IPA interface if available
		if (ipaInterface_) {
			LOG(SoftISPPipeline, Info) << "Setting IPA interface on virtual camera";
			frameGenerator_->setIpaInterface(ipaInterface_);
		} else {
			LOG(SoftISPPipeline, Info) << "No IPA interface available for virtual camera";
		}
		
		// Set callback for frame completion
		frameGenerator_->setFrameDoneCallback(
		    [](unsigned int frameId, unsigned int bufferId) {
		        // Handle frame completion
		        LOG(SoftISPPipeline, Info) << "Virtual camera frame done: frameId=" << frameId 
		                                   << ", bufferId=" << bufferId;
		    });
	}
	
	return frameGenerator_ != nullptr;
}
