/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <libcamera/formats.h>
#include "softisp.h"

std::unique_ptr<CameraConfiguration>
SoftISPCameraData::generateConfiguration(Span<const StreamRole> roles)
{
	auto config = std::make_unique<SoftISPConfiguration>();

	for (const auto &role : roles) {
		(void)role;
		StreamConfiguration streamCfg;
		streamCfg.size = Size(1920, 1080);
		streamCfg.pixelFormat = formats::SBGGR10;
		streamCfg.bufferCount = 4;
		config->addConfiguration(streamCfg);
	}

	return config;
}
