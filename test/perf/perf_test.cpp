#include <chrono>
#include <iostream>
#include <memory>

#include "libcamera/camera_manager.h"
#include "libcamera/camera.h"
#include "libcamera/request.h"
#include "libcamera/stream.h"

using namespace libcamera;
using namespace std::chrono;

int main()
{
    std::cout << "=== SoftISP Pipeline Performance Test (C++) ===" << std::endl;
    
    // Initialize camera manager
    auto cm = std::make_unique<CameraManager>();
    if (cm->start()) {
        std::cerr << "Failed to start camera manager" << std::endl;
        return -1;
    }
    
    // List available cameras
    std::cout << "Available cameras:" << std::endl;
    for (const auto &camId : cm->cameras()) {
        std::cout << "  - " << camId << std::endl;
    }
    
    // Performance timing
    auto start = high_resolution_clock::now();
    
    // Test camera enumeration performance
    const size_t iterations = 100;
    for (size_t i = 0; i < iterations; ++i) {
        auto cameras = cm->cameras();
        if (i % 20 == 0) {
            std::cout << "Enumerated cameras (" << i << "/" << iterations << ")" << std::endl;
        }
    }
    
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    
    std::cout << "Camera enumeration performance: " << duration.count() << " μs for " 
              << iterations << " iterations" << std::endl;
    std::cout << "Average per iteration: " << (duration.count() / iterations) << " μs" << std::endl;
    
    cm->stop();
    std::cout << "✓ Performance test completed successfully!" << std::endl;
    return 0;
}
