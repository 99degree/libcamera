/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <libcamera/stream.h>
#include <libcamera/formats.h>

namespace libcamera {

// A minimal placeholder stream for virtual cameras
class PlaceholderStream : public Stream {
public:
	PlaceholderStream() {
		configuration_.pixelFormat = formats::SBGGR10;
		configuration_.size = Size(1920, 1080);
		configuration_.stride = (1920 * 10 + 7) / 8;
		configuration_.frameSize = configuration_.stride * 1080;
		configuration_.bufferCount = 4;
	}
};

} /* namespace libcamera */
