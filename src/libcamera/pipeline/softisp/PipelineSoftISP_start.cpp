/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int PipelineHandlerSoftISP::start(Camera *camera, const ControlList *controls)
{
	SoftISPCameraData *data = cameraData(camera);
	if (!data) {
		LOG(SoftISPPipeline, Error) << "Failed to get camera data";
		return -EINVAL;
	}
	return data->start(controls);
}
