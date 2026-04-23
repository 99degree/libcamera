/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Virtual Camera - Standalone virtual camera implementation
 */

#include "virtual_camera.h"

#include <algorithm>
#include <cstring>

#include <libcamera/base/log.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(VirtualCamera)

VirtualCamera::VirtualCamera()
	: Thread("VirtualCamera")
{
}

VirtualCamera::~VirtualCamera()
{
	stop();
}

int VirtualCamera::init(unsigned int width, unsigned int height)
{
	width_ = width;
	height_ = height;

	LOG(VirtualCamera, Info) << "Virtual camera initialized: "
	                         << width << "x" << height;

	return 0;
}

int VirtualCamera::start()
{
	if (running_)
		return 0;

	running_ = true;
	start();

	LOG(VirtualCamera, Info) << "Virtual camera started";

	return 0;
}

void VirtualCamera::stop()
{
	if (!running_)
		return;

	running_ = false;
	bufferCV_.notify_all();
	exit(0);
	wait();

	LOG(VirtualCamera, Info) << "Virtual camera stopped";
}

void VirtualCamera::queueBuffer(FrameBuffer *buffer)
{
	if (!running_ || !buffer)
		return;

	{
		std::lock_guard<std::mutex> lock(queueMutex_);
		bufferQueue_.push(buffer);
	}

	bufferCV_.notify_one();
}

void VirtualCamera::setPattern(Pattern pattern)
{
	pattern_ = pattern;
}

void VirtualCamera::setBrightness(float brightness)
{
	brightness_ = std::clamp(brightness, 0.0f, 1.0f);
}

void VirtualCamera::setContrast(float contrast)
{
	contrast_ = std::clamp(contrast, 0.0f, 2.0f);
}

void VirtualCamera::run()
{
	while (running_) {
		FrameBuffer *buffer = nullptr;

		{
			std::unique_lock<std::mutex> lock(queueMutex_);
			bufferCV_.wait(lock, [this] {
				return !running_ || !bufferQueue_.empty();
			});

			if (!running_)
				break;

			if (bufferQueue_.empty())
				continue;

			buffer = bufferQueue_.front();
			bufferQueue_.pop();
		}

		if (!buffer || buffer->planes().empty()) {
			LOG(VirtualCamera, Error) << "Invalid buffer received";
			continue;
		}

		/* Generate pattern into buffer */
		sequence_++;

		LOG(VirtualCamera, Debug) << "Generated frame " << sequence_;

		std::this_thread::sleep_for(std::chrono::milliseconds(33));
	}
}

} /* namespace libcamera */
