#include "virtual_camera.h"

#include <libcamera/base/log.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(VirtualCamera)

VirtualCamera::VirtualCamera()
    : running_(false)
{
    LOG(VirtualCamera, Info) << "Virtual camera initialized: " << width_ << "x" << height_;
}

VirtualCamera::~VirtualCamera()
{
    stop();
    LOG(VirtualCamera, Info) << "Virtual camera destroyed";
}

int VirtualCamera::start()
{
    if (running_)
        return 0;
    
    LOG(VirtualCamera, Info) << "Starting VirtualCamera";
    running_ = true;
    Thread::start();
    return 0;
}

void VirtualCamera::stop()
{
    if (!running_)
        return;
    
    LOG(VirtualCamera, Info) << "Stopping VirtualCamera";
    running_ = false;
    exit();
    wait();
}

int VirtualCamera::init(unsigned int width, unsigned int height)
{
    width_ = width;
    height_ = height;
    LOG(VirtualCamera, Info) << "Initialized: " << width << "x" << height;
    return 0;
}

int VirtualCamera::queueBuffer(FrameBuffer *buffer)
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    bufferQueue_.push(buffer);
    return 0;
}

void VirtualCamera::generateFrame()
{
    static int frameCount = 0;
    frameCount++;
    LOG(VirtualCamera, Debug) << "Frame " << frameCount << " generated";
}

void VirtualCamera::run()
{
    LOG(VirtualCamera, Info) << "VirtualCamera thread running";
    
    while (running_) {
        generateFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    
    LOG(VirtualCamera, Info) << "VirtualCamera thread exiting";
}

} // namespace libcamera
