/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

#include <libcamera/formats.h>
#include <libcamera/pixel_format.h>

CameraConfiguration::Status SoftISPConfiguration::validate()
{
	Status status = Valid;

	if (config_.empty())
		return Invalid;

	/*
	 * Our virtual camera supports raw Bayer 10-bit SBGGR10 at 1920x1080.
	 * For simplicity, we adjust all stream configurations to this format.
	 */
	for (auto &cfg : config_) {
		/* Set pixel format to raw Bayer 10-bit */
		if (cfg.pixelFormat != formats::SBGGR10) {
			cfg.pixelFormat = formats::SBGGR10;
			status = Adjusted;
		}

		/* Set size to our virtual camera resolution */
		if (cfg.size.width != 1920 || cfg.size.height != 1080) {
			cfg.size.width = 1920;
			cfg.size.height = 1080;
			status = Adjusted;
		}

		/* Calculate stride and frame size for 10-bit packed Bayer */
		cfg.stride = 1920 * 10 / 8;  /* 2400 bytes per line */
		cfg.frameSize = cfg.stride * 1080;

		/* Set buffer count */
		if (cfg.bufferCount < 4)
			cfg.bufferCount = 4;
	}

	return status;
}