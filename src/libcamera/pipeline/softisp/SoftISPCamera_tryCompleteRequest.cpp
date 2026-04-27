/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftISPCameraData::tryCompleteRequest(SoftISPFrameInfo *info)
{
	if (!info || !info->request) {
		return;
	}
	
	// Check if both metadata and frame are ready
	if (info->metadataReceived && info->frameReceived) {
		// Add metadata to the request
		if (!info->metadata.empty()) {
			info->request->controls().merge(info->metadata, ControlList::MergePolicy::OverwriteExisting);
			LOG(SoftISPPipeline, Debug) << "Added metadata to request for frame " << info->frame;
		}
		
		// Complete the request
		pipe()->completeRequest(info->request);
		
		// Remove from frame info
		frameInfo_.destroy(info->frame);
	}
}
