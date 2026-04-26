/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::configure([[maybe_unused]] CameraConfiguration *config)
{
	LOG(SoftISPPipeline, Info) << "Configuring camera";
	if (!frameGenerator_) {
		LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
		return -EINVAL;
	}
	return 0;
}
