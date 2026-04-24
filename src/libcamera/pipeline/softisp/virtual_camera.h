/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Virtual Camera - Standalone virtual camera implementation
 * Provides test pattern generation and frame streaming for testing
 */
#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <libcamera/base/thread.h>
#include <libcamera/base/mutex.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/framebuffer.h"

namespace libcamera {

class VirtualCamera : public Thread {
public:
	enum class Pattern {
		SolidColor,
		Grayscale,
		ColorBars,
		Checkerboard,
		SineWave,
	};

	VirtualCamera();
	~VirtualCamera();

	int init(unsigned int width, unsigned int height);
	int start();
	void stop();
	void queueBuffer(FrameBuffer *buffer);
	void setPattern(Pattern pattern);
	void setBrightness(float brightness);
	void setContrast(float contrast);

	unsigned int sequence() const { return sequence_; }

	/* Accessors for buffer size */
	unsigned int width() const { return width_; }
	unsigned int height() const { return height_; }
	unsigned int bufferCount() const { return 4; }

private:
	void run() override;

	unsigned int width_ = 0;
	unsigned int height_ = 0;
	Pattern pattern_ = Pattern::ColorBars;
	float brightness_ = 0.5f;
	float contrast_ = 1.0f;

	bool running_ = false;
	unsigned int sequence_ = 0;

	std::mutex queueMutex_;
	std::queue<FrameBuffer*> bufferQueue_;
	std::condition_variable bufferCV_;
};

} /* namespace libcamera */
