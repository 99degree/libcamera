#include <libcamera/libcamera.h>
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    libcamera::CameraManager cm;
    cm.start();
    
    std::cout << "Available cameras:" << std::endl;
    for (const auto &camera : cm.cameras()) {
        std::cout << "  - " << camera->id() << std::endl;
    }
    
    if (cm.cameras().empty()) {
        std::cout << "No cameras found!" << std::endl;
        return 1;
    }
    
    auto camera = cm.cameras()[0];
    std::cout << "Selected camera: " << camera->id() << std::endl;
    
    auto config = camera->generateConfiguration({libcamera::StreamRole::Raw});
    if (!config) {
        std::cout << "Failed to generate configuration" << std::endl;
        return 1;
    }
    
    (*config)[0].size = libcamera::Size(1920, 1080);
    config->validate();
    
    if (camera->configure(config.get()) < 0) {
        std::cout << "Failed to configure camera" << std::endl;
        return 1;
    }
    
    std::cout << "Camera configured successfully!" << std::endl;
    
    libcamera::FrameBufferAllocator allocator(camera);
    for (const auto &stream : camera->streams()) {
        if (allocator.allocate(stream) < 0) {
            std::cout << "Failed to allocate buffers" << std::endl;
            return 1;
        }
    }
    
    std::cout << "Buffers allocated!" << std::endl;
    
    if (camera->start() < 0) {
        std::cout << "Failed to start camera" << std::endl;
        return 1;
    }
    
    std::cout << "Camera started!" << std::endl;
    
    for (int i = 0; i < 3; i++) {
        auto request = camera->createRequest();
        if (!request) {
            std::cout << "Failed to create request" << std::endl;
            return 1;
        }
        
        const auto &buffers = allocator.buffers(camera->streams()[0]);
        for (const auto &buffer : buffers) {
            if (request->addBuffer(camera->streams()[0], buffer.get()) < 0) {
                std::cout << "Failed to add buffer to request" << std::endl;
                return 1;
            }
        }
        
        if (camera->queueRequest(request.get()) < 0) {
            std::cout << "Failed to queue request" << std::endl;
            return 1;
        }
        
        std::cout << "Queued request " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    camera->stop();
    std::cout << "Camera stopped!" << std::endl;
    
    cm.stop();
    std::cout << "Test completed successfully!" << std::endl;
    
    return 0;
}
