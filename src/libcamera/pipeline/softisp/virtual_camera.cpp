/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "virtual_camera.h"

#include <algorithm>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>
#include <libcamera/controls.h>


LOG_DEFINE_CATEGORY(VirtualCamera)

VirtualCamera::VirtualCamera()
    : Thread("VirtualCamera")
{
}

VirtualCamera::~VirtualCamera()
{
    stop();
    releaseBuffers();
}

int VirtualCamera::init(unsigned int width, unsigned int height)
{
    if (running_) {
        LOG(VirtualCamera, Warning) << "VirtualCamera already running";
        return -EBUSY;
    }
    
    width_ = width;
    height_ = height;
    
    LOG(VirtualCamera, Info) << "Virtual camera initialized: " << width << "x" << height;
    
    return 0;
}

int VirtualCamera::start()
{
    if (running_) {
        return 0;
    }
    
    running_ = true;
    Thread::start();
    
    LOG(VirtualCamera, Info) << "Virtual camera started";
    
    return 0;
}

void VirtualCamera::stop()
{
    if (!running_) {
        return;
    }
    
    running_ = false;
    bufferCV_.notify_all();
    
    Thread::exit();
    wait();
    
    LOG(VirtualCamera, Info) << "Virtual camera stopped";
}

int VirtualCamera::allocateBuffers(unsigned int count)
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    
    if (!buffers_.empty()) {
        LOG(VirtualCamera, Warning) << "Buffers already allocated";
        return -EBUSY;
    }
    
    const unsigned int bufferSize = width_ * height_ * 10 / 8;
    
    LOG(VirtualCamera, Info) << "Allocating " << count << " buffers of " 
                              << bufferSize << " bytes each";
    
    bufferInUse_.resize(count, false);
    
    for (unsigned int i = 0; i < count; i++) {
        int fd = memfd_create("virtual_camera_buffer", MFD_CLOEXEC);
        if (fd < 0) {
            LOG(VirtualCamera, Error) << "Failed to create buffer fd";
            releaseBuffers();
            return -errno;
        }
        
        if (ftruncate(fd, bufferSize) < 0) {
            LOG(VirtualCamera, Error) << "Failed to set buffer size";
            close(fd);
            releaseBuffers();
            return -errno;
        }
        
        FrameBuffer::Plane plane;
        plane.fd = SharedFD(fd);
        plane.length = bufferSize;
        plane.offset = 0;
        
        std::vector<FrameBuffer::Plane> planes;
        planes.push_back(std::move(plane));
        
        bufferFds_.push_back(fd);
        auto buffer = new FrameBuffer(planes);
        buffers_.push_back(buffer);
    }
    
    LOG(VirtualCamera, Info) << "Successfully allocated " << buffers_.size() << " buffers";
    
    return 0;
}

void VirtualCamera::releaseBuffers()
{
    std::lock_guard<std::mutex> lock(bufferMutex_);
    
    for (auto *buffer : buffers_) {
        delete buffer;
    }
    
    buffers_.clear();
    bufferFds_.clear();
    bufferInUse_.clear();
}

std::vector<FrameBuffer*>& VirtualCamera::getBuffers()
{
    return buffers_;
}

void VirtualCamera::queueRequest(Request *request)
{
    if (!running_ || !request) {
        LOG(VirtualCamera, Warning) << "Invalid request";
        return;
    }
    
    const Request::BufferMap &bufferMap = request->buffers();
    if (bufferMap.empty()) {
        LOG(VirtualCamera, Warning) << "No buffers in request";
        return;
    }
    
    FrameBuffer *buffer = bufferMap.begin()->second;
    if (!buffer) {
        LOG(VirtualCamera, Warning) << "Null buffer in request";
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        requestQueue_.push({request, 0});
    }
    
    bufferCV_.notify_one();
}

void VirtualCamera::setFrameDoneCallback(
    std::function<void(unsigned int, unsigned int)> callback)
{
    frameDoneCallback_ = callback;
}

void VirtualCamera::setPattern(Pattern pattern) { pattern_ = pattern; }
void VirtualCamera::setBrightness(float brightness) { brightness_ = std::clamp(brightness, 0.0f, 2.0f); }
void VirtualCamera::setContrast(float contrast) { contrast_ = std::clamp(contrast, 0.0f, 2.0f); }
void VirtualCamera::setSaturation(float saturation) { saturation_ = std::clamp(saturation, 0.0f, 2.0f); }
void VirtualCamera::setSharpness(float sharpness) { sharpness_ = std::clamp(sharpness, 0.0f, 2.0f); }

bool VirtualCamera::hasAvailableBuffer()
{
    std::lock_guard<std::mutex> lock(bufferUsageMutex_);
    
    for (bool inUse : bufferInUse_) {
        if (!inUse) {
            return true;
        }
    }
    
    return false;
}

ControlList VirtualCamera::generateMetadata([[maybe_unused]] unsigned int frame)
{
    ControlList metadata;
    return metadata;
}

void VirtualCamera::processFrame(FrameBuffer *buffer, [[maybe_unused]] Request *request)
{
    if (!buffer || buffer->planes().empty()) {
        LOG(VirtualCamera, Error) << "Invalid buffer";
        return;
    }

    {
        std::lock_guard<std::mutex> lock(bufferUsageMutex_);
        for (size_t i = 0; i < buffers_.size(); i++) {
            if (buffers_[i] == buffer) {
                bufferInUse_[i] = true;
                break;
            }
        }
    }

    const auto &plane = buffer->planes()[0];
    void *mem = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE, 
                     MAP_SHARED, plane.fd.get(), 0);
    
    if (mem == MAP_FAILED) {
        LOG(VirtualCamera, Error) << "Failed to map buffer";
        return;
    }

    sequence_++;
    auto _p_start = std::chrono::steady_clock::now();
    LOG(VirtualCamera, Info) << "Processing frame " << sequence_ 
                              << " (Bayer10 RGGB " << width_ << "x" << height_ << ")";

    uint8_t *data = static_cast<uint8_t*>(mem);

    for (unsigned int y = 0; y < height_; y++) {
        for (unsigned int x = 0; x < width_; x++) {
            uint16_t pixelValue;
            
            if ((x % 2 == 0) && (y % 2 == 0)) {
                pixelValue = 0x200 + ((x * 3) % 256);
            } else if ((x % 2 == 1) && (y % 2 == 0)) {
                pixelValue = 0x300 + ((y * 5) % 256);
            } else if ((x % 2 == 0) && (y % 2 == 1)) {
                pixelValue = 0x300 + ((y * 5) % 256);
            } else {
                pixelValue = 0x400 + ((x * 3) % 256);
            }

            unsigned int bitOffset = (y * width_ + x) * 10;
            unsigned int byteOffset = bitOffset / 8;
            unsigned int bitInByte = bitOffset % 8;

            if (byteOffset + 1 < plane.length) {
                data[byteOffset] &= ~(0x3 << bitInByte);
                data[byteOffset] |= (pixelValue & 0x3F) << bitInByte;

                if (bitInByte > 6) {
                    data[byteOffset + 1] &= ~(0x3F >> (8 - bitInByte));
                    data[byteOffset + 1] |= (pixelValue >> (8 - bitInByte)) & 
                                           (0x3F >> (8 - bitInByte));
                }
            }
        }
    }

    munmap(mem, plane.length);

    ControlList metadata = generateMetadata(sequence_);

    if (frameDoneCallback_) {
        uint32_t bufferId = plane.fd.get();
        frameDoneCallback_(sequence_, bufferId);
    }

    auto _p_end = std::chrono::steady_clock::now();
    auto _p_us = std::chrono::duration_cast<std::chrono::microseconds>(_p_end - _p_start).count();
    LOG(VirtualCamera, Info) << "[VCAM] frame=" << sequence_ << " took " << _p_us << "us";

    {
        std::lock_guard<std::mutex> lock(bufferUsageMutex_);
        for (size_t i = 0; i < buffers_.size(); i++) {
            if (buffers_[i] == buffer) {
                bufferInUse_[i] = false;
                break;
            }
        }
    }
}



void VirtualCamera::run()
{
    LOG(VirtualCamera, Info) << "VirtualCamera::run() thread started";
    
    while (running_) {
        Request *request = nullptr;
        FrameBuffer *buffer = nullptr;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            bufferCV_.wait(lock, [this] { 
                return !running_ || !requestQueue_.empty(); 
            });
            
            if (!running_) break;
            if (requestQueue_.empty()) continue;
            
            auto [req, bufIndex] = requestQueue_.front();
            request = req;
            
            const Request::BufferMap &bufferMap = request->buffers();
            if (!bufferMap.empty()) {
                buffer = bufferMap.begin()->second;
            }
            
            requestQueue_.pop();
        }
        
        if (!hasAvailableBuffer()) {
            skippedFrames_++;
            LOG(VirtualCamera, Warning) << "All buffers in use, skipping frame " 
                                        << (sequence_ + 1) << " (skipped: " 
                                        << skippedFrames_ << " total)";
            continue;
        }
        
        if (buffer) {
            /* Throttle IPA: only process every 4th frame through IPA.
             * processFrame runs every frame at full speed. */
            if (ipaInterface_ && (sequence_ % 4 == 0)) {
                processWithIPA(buffer, request);
            }
            
            processFrame(buffer, request);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    LOG(VirtualCamera, Info) << "processWithIPA called - IPA interface available: " << (ipaInterface_ ? "yes" : "no");
    LOG(VirtualCamera, Info) << "VirtualCamera::run() thread exiting";
}

void VirtualCamera::processWithIPA(FrameBuffer *buffer, [[maybe_unused]] Request *request)
{
    if (!ipaInterface_ || !buffer || buffer->planes().empty()) {
        LOG(VirtualCamera, Warning) << "IPA processing skipped - no interface or invalid buffer";
        return;
    }

    LOG(VirtualCamera, Info) << "Processing frame with IPA";

    const auto &plane = buffer->planes()[0];
    uint32_t bufferId = plane.fd.get();
    uint32_t frameId = sequence_;
    
    // Create control list for sensor controls (empty for now)
    ControlList sensorControls;
    
    // Call IPA processFrame method
    ipaInterface_->processFrame(
        frameId,           // frame
        bufferId,          // bufferId
        plane.fd,           // bufferFd
        0,                 // planeIndex
        width_,            // width
        height_,          // height
        sensorControls    // results
    );
    
    LOG(VirtualCamera, Info) << "IPA processFrame called for frame " << frameId << ", bufferId " << bufferId;
}
