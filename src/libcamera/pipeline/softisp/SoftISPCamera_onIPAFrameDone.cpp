/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

void SoftISPCameraData::onIPAFrameDone(uint32_t frame, uint32_t bufferId)
{
	LOG(SoftISPPipeline, Info) << "IPA frame done: frame=" << frame << ", bufferId=" << bufferId;
	
	// Call the pipeline's frameDone to notify the Camera/App
	frameDone(frame, bufferId);
}

void SoftISPCameraData::onIPAMetadataReady(uint32_t frame, const ControlList &metadata)
{
	LOG(SoftISPPipeline, Info) << "IPA metadata ready: frame=" << frame;
	
	// Store metadata in the frame info
	SoftISPFrameInfo *frameInfo = frameInfo_.find(frame);
	if (frameInfo) {
		frameInfo->metadata = metadata;
		frameInfo->metadataReceived = true;
		
		// Try to complete the request if buffer is also done
		tryCompleteRequest(frameInfo);
	}
}

} /* namespace libcamera */
