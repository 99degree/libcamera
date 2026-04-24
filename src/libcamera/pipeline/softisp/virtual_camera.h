/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024 George Chan <gchan9527@gmail.com>
 *
 * Virtual Camera - Standalone virtual camera implementation
 */

#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <libcamera/base/thread.h>
#include <libcamera/base/mutex.h>
#include <libcamera/internal/camera.h>

namespace libcamera {

class FrameBuffer;

enum class Pattern {
    ColorBars,
    GrayScale,
    Red,
    Green,
    Blue,
};

class VirtualCamera : public Thread
{
public:
    VirtualCamera();
    ~VirtualCamera();

    int start();
    void stop();
    
    // Basic initialization
    int init(unsigned int width, unsigned int height);
    
    // Frame buffer management
    int queueBuffer(FrameBuffer *buffer);
    void generateFrame();

private:
    void run() override;

    unsigned int width_ = 1920;
    unsigned int height_ = 1080;
    
public:
    unsigned int width() const { return width_; }
    unsigned int height() const { return height_; }
    unsigned int bufferCount() const { return 2; }
    bool running_ = false;
    std::mutex queueMutex_;
    std::queue<FrameBuffer*> bufferQueue_;
    std::vector<std::unique_ptr<FrameBuffer>> frameBuffers_;
};

} // namespace libcamera
