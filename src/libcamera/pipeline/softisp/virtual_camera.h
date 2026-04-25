/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, SoftISP Project
 *
 * VirtualCamera - Standalone virtual camera implementation
 * Can optionally route frames through IPA for processing.
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

// Forward declare IPA interface
namespace libcamera {
namespace ipa {
namespace soft {
class IPASoftInterface;
}
}
}

namespace libcamera {


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

    
    // Set IPA interface for frame processing
    
    void queueRequest(Request *request);
    void setFrameDoneCallback(std::function<void(unsigned int frameId, unsigned int bufferId)> callback);
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
    unsigned int skippedFrames() const { return skippedFrames_; }

protected:
    void run() override;

private:
    void processFrame(FrameBuffer *buffer, Request *request);
    void processWithIPA(FrameBuffer *buffer, Request *request);
    ControlList generateMetadata(unsigned int frame);
    bool hasAvailableBuffer();

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
    unsigned int skippedFrames_ = 0;
    
    std::vector<FrameBuffer*> buffers_;
    std::vector<int> bufferFds_;
    std::mutex bufferMutex_;
    
    // Track which buffers are currently in use
    std::vector<bool> bufferInUse_;
    std::mutex bufferUsageMutex_;
    
    // IPA interface for frame processing
    
    std::function<void(unsigned int, unsigned int)> frameDoneCallback_;
};

} /* namespace libcamera */
