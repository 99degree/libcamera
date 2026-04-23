/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Minimal virtual camera helper for testing and dummy pipelines
 */

#pragma once

#include <libcamera/camera.h>
#include <libcamera/base/thread.h>
#include <libcamera/base/log.h>

#include <mutex>
#include <queue>

namespace libcamera {

class PipelineHandler;

/**
 * @brief Minimal virtual camera helper for testing
 *
 * A tiny, standalone class that provides virtual camera functionality
 * without any dependencies on specific pipelines.
 *
 * This class handles:
 * - Thread-based request processing
 * - Basic buffer management
 * - Frame generation hook (override to customize)
 *
 * Usage:
 * @code
 * class MyDummyCameraData : public Camera::Private, public VirtualCameraStub {
 * public:
 *     MyDummyCameraData(PipelineHandler *pipe)
 *         : Camera::Private(pipe), VirtualCameraStub(pipe) {}
 *
 *     int init() {
 *         // Your initialization
 *         return 0;
 *     }
 *
 * protected:
 *     int generateFrame(FrameBuffer *buffer, uint32_t sequence) override {
 *         // Custom frame generation
 *         // e.g., generate test pattern, load image, etc.
 *         return 0;
 *     }
 * };
 * @endcode
 */
class VirtualCameraStub : public Thread {
public:
    explicit VirtualCameraStub(PipelineHandler *pipe)
        : Thread("VirtualCam"), pipe_(pipe),
          running_(false), sequence_(0)
    {
    }

    virtual ~VirtualCameraStub()
    {
        stop();
    }

    /**
     * @brief Start the virtual camera
     * @return 0 on success, negative error code
     */
    int start()
    {
        if (running_)
            return 0;

        running_ = true;
        Thread::start();
        return 0;
    }

    /**
     * @brief Stop the virtual camera
     */
    void stop()
    {
        if (!running_)
            return;

        running_ = false;
        Thread::stop();
    }

    /**
     * @brief Queue a request for processing
     * @param request Request to queue
     */
    void queueRequest(Request *request)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingRequests_.push(request);
    }

    /**
     * @brief Check if camera is running
     * @return true if running
     */
    bool isRunning() const
    {
        return running_;
    }

protected:
    /**
     * @brief Thread entry point
     */
    void run() override
    {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            std::lock_guard<std::mutex> lock(mutex_);
            while (!pendingRequests_.empty()) {
                Request *req = pendingRequests_.front();
                pendingRequests_.pop();
                processRequest(req);
            }
        }
    }

    /**
     * @brief Process a single request
     * @param request Request to process
     *
     * Default implementation calls generateFrame() and completes the request.
     * Override this method for custom behavior.
     */
    virtual void processRequest(Request *request)
    {
        const auto &buffers = request->buffers();
        if (buffers.empty()) {
            request->complete(-EINVAL);
            return;
        }

        FrameBuffer *buffer = buffers[0].buffer;
        int ret = generateFrame(buffer, sequence_++);
        request->complete(ret);
    }

    /**
     * @brief Generate frame data for a buffer
     * @param buffer Buffer to fill
     * @param sequence Frame sequence number
     * @return 0 on success, negative error code
     *
     * Override this method to provide custom frame generation:
     * - Test patterns (gradient, checkerboard, etc.)
     * - Load images from disk
     * - Generate synthetic data
     * - Read from a file or network
     */
    virtual int generateFrame(FrameBuffer *buffer, uint32_t sequence)
    {
        // Default: do nothing (buffer remains unchanged)
        // Override in derived class for actual frame generation
        return 0;
    }

    PipelineHandler *pipe() const
    {
        return pipe_;
    }

    uint32_t sequence() const
    {
        return sequence_;
    }

private:
    PipelineHandler *pipe_;
    bool running_;
    uint32_t sequence_;
    std::queue<Request *> pendingRequests_;
    std::mutex mutex_;
};

} // namespace libcamera
