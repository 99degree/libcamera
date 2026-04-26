/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftISPCameraData::metadataReady(unsigned int frame, const ControlList &metadata)
{
	LOG(SoftISPPipeline, Info) << "Metadata ready for frame " << frame;
	
	// Store metadata in the frame info
	SoftISPFrameInfo *frameInfo = frameInfo_.find(frame);
	if (frameInfo) {
		frameInfo->metadataReceived = true;
		// TODO: Store the actual metadata if needed
		(void)metadata;  // Mark as intentionally unused for now
		
		// Try to complete the request if frame is also ready
		tryCompleteRequest(frameInfo);
	}
}
