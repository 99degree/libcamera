/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "virtual_camera.h"
#include <libcamera/controls.h>


#include <algorithm>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>

namespace libcamera {

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
    Thread::start();  // Call base class start
    
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
    
    Thread::exit();  // Signal thread to exit
    wait();          // Wait for thread to finish
    
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
    
    for (unsigned int i = 0; i < count; i++) {
        auto buffer = new FrameBuffer();
        
        int fd = memfd_create("virtual_camera_buffer", MFD_CLOEXEC);
        if (fd < 0) {
            LOG(VirtualCamera, Error) << "Failed to create buffer fd";
            releaseBuffers();
            return -errno;
        }
        
        if (ftruncate(fd, bufferSize) < 0) {
            LOG(VirtualCamera, Error) << "Failed to set buffer size";
            close(fd);
            delete buffer;
            releaseBuffers();
            return -errno;
        }
        
        FrameBuffer::Plane plane;
        plane.fd = std::unique_ptr<FD>(new FD(fd));
        plane.length = bufferSize;
        plane.memoryOffset = 0;
        
        buffer->planes().push_back(std::move(plane));
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
    std::function<void(Request *request, ControlList &metadata)> callback)
{
    frameDoneCallback_ = callback;
}

void VirtualCamera::setPattern(Pattern pattern) { pattern_ = pattern; }
void VirtualCamera::setBrightness(float brightness) { brightness_ = std::clamp(brightness, 0.0f, 2.0f); }
void VirtualCamera::setContrast(float contrast) { contrast_ = std::clamp(contrast, 0.0f, 2.0f); }
void VirtualCamera::setSaturation(float saturation) { saturation_ = std::clamp(saturation, 0.0f, 2.0f); }
void VirtualCamera::setSharpness(float sharpness) { sharpness_ = std::clamp(sharpness, 0.0f, 2.0f); }

ControlList VirtualCamera::generateMetadata([[maybe_unused]] unsigned int frame)
{
    ControlList metadata(controls::controls);
    metadata.add(libcamera::controls::FrameDuration, 33333LL);
    metadata.add(libcamera::controls::SensorTemperature, 35.0f);
    metadata.add(libcamera::controls::LensPosition, 1.0f);
    return metadata;
}

void VirtualCamera::processFrame(FrameBuffer *buffer, Request *request)
{
    if (!buffer || buffer->planes().empty()) {
        LOG(VirtualCamera, Error) << "Invalid buffer";
        return;
    }
    
    const auto &plane = buffer->planes()[0];
    void *mem = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE, 
                     MAP_SHARED, plane.fd.get(), 0);
    
    if (mem == MAP_FAILED) {
        LOG(VirtualCamera, Error) << "Failed to map buffer";
        return;
    }
    
    sequence_++;
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
        frameDoneCallback_(request, metadata);
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
        
        if (buffer) {
            processFrame(buffer, request);
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    LOG(VirtualCamera, Info) << "VirtualCamera::run() thread exiting";
}

} /* namespace libcamera */
