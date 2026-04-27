/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftISPCameraData::metadataReady(unsigned int frame, const ControlList &metadata)
{
	LOG(SoftISPPipeline, Info) << "Metadata ready for frame " << frame;
	
	// Store the latest metadata in the camera data
	latestMetadata_ = metadata;
	LOG(SoftISPPipeline, Debug) << "Updated latest metadata";
	
	// Store metadata in the frame info
	SoftISPFrameInfo *frameInfo = frameInfo_.find(frame);
	if (frameInfo) {
		frameInfo->metadata = metadata;
		frameInfo->metadataReceived = true;
		LOG(SoftISPPipeline, Debug) << "Stored metadata for frame " << frame;
		// Try to complete the request if frame is also ready
		tryCompleteRequest(frameInfo);
	} else {
		LOG(SoftISPPipeline, Warning) << "No frame info for metadata, frame=" << frame;
	}
}