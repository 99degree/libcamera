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
#include "libcamera/stream.h"

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

	int init(unsigned int width, unsigned int height, PixelFormat format);
	int start();
	void stop();
	void queueBuffer(FrameBuffer *buffer);
	void setPattern(Pattern pattern);
	void setBrightness(float brightness);
	void setContrast(float contrast);

	/**
	 * Get the current frame sequence number
	 */
	unsigned int sequence() const { return sequence_; }

private:
	void run() override;
	void generatePattern(uint8_t *data, unsigned int width, unsigned int height,
	                     unsigned int stride);
	void generateSolidColor(uint8_t *data, unsigned int size);
	void generateGrayscale(uint8_t *data, unsigned int width, unsigned int height,
	                       unsigned int stride);
	void generateColorBars(uint8_t *data, unsigned int width, unsigned int height,
	                       unsigned int stride);
	void generateCheckerboard(uint8_t *data, unsigned int width, unsigned int height,
	                          unsigned int stride);
	void generateSineWave(uint8_t *data, unsigned int width, unsigned int height,
	                      unsigned int stride);

	unsigned int width_ = 0;
	unsigned int height_ = 0;
	PixelFormat format_ = formats::UYVY888;
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
