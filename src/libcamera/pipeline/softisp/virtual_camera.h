/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, SoftISP Project
 *
 * VirtualCamera - Standalone virtual camera implementation
 * Inherits from libcamera::Thread for standardized threading.
 * 
 * Each instance is fully independent with its own state,
 * allowing multiple virtual cameras to run simultaneously.
 */

#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include <libcamera/base/log.h>
#include <libcamera/base/thread.h>
#include <libcamera/framebuffer.h>
#include <libcamera/request.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(VirtualCamera)

class VirtualCamera : public Thread
{
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

    int allocateBuffers(unsigned int count);
    void releaseBuffers();
    std::vector<FrameBuffer*>& getBuffers();

    void queueRequest(Request *request);
    void setFrameDoneCallback(std::function<void(Request *request, ControlList &metadata)> callback);

    void setPattern(Pattern pattern);
    void setBrightness(float brightness);
    void setContrast(float contrast);
    void setSaturation(float saturation);
    void setSharpness(float sharpness);

    unsigned int width() const { return width_; }
    unsigned int height() const { return height_; }
    unsigned int bufferCount() const { return buffers_.size(); }
    bool isRunning() const { return running_; }
    unsigned int sequence() const { return sequence_; }

protected:
    void run() override;

private:
    void processFrame(FrameBuffer *buffer, Request *request);
    ControlList generateMetadata(unsigned int frame);

    unsigned int width_ = 0;
    unsigned int height_ = 0;
    bool running_ = false;
    
    Pattern pattern_ = Pattern::SolidColor;
    float brightness_ = 1.0f;
    float contrast_ = 1.0f;
    float saturation_ = 1.0f;
    float sharpness_ = 1.0f;
    
    std::mutex queueMutex_;
    std::condition_variable bufferCV_;
    std::queue<std::pair<Request*, unsigned int>> requestQueue_;
    
    unsigned int sequence_ = 0;
    
    std::vector<FrameBuffer*> buffers_;
    std::mutex bufferMutex_;
    
    std::function<void(Request *request, ControlList &metadata)> frameDoneCallback_;
};

} /* namespace libcamera */
