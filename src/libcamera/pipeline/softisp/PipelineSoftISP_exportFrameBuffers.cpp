/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int PipelineHandlerSoftISP::exportFrameBuffers(
	Camera *camera, Stream *stream,
	std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	SoftISPCameraData *data = cameraData(camera);
	if (!data) {
		LOG(SoftISPPipeline, Error) << "Failed to get camera data";
		return -EINVAL;
	}
	return data->exportFrameBuffers(stream, buffers);
}
