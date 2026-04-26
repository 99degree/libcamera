/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftISPCameraData::tryCompleteRequest(SoftISPFrameInfo *info)
{
	if (!info || !info->request) {
		return;
	}
	
	// Check if both metadata and frame are ready
	if (info->metadataReceived && info->frameReceived) {
		// Complete the request
		pipe()->completeRequest(info->request);
		
		// Remove from frame info
		frameInfo_.destroy(info->frame);
	}
}
