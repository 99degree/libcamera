#include <iostream>
#include <libcamera/libcamera.h>

int main() {
    libcamera::CameraManager cm;
    cm.start();
    
    auto cameras = cm.cameras();
    std::cout << "Found " << cameras.size() << " cameras" << std::endl;
    
    for (const auto &cam : cameras) {
        std::cout << "Camera: " << cam->id() << std::endl;
        
        auto config = cam->generateConfiguration({libcamera::StreamRole::Viewfinder});
        if (config) {
            std::cout << "  Config generated, size: " << config->size() << std::endl;
            auto result = config->validate();
            std::cout << "  Validation: " << result << std::endl;
        } else {
            std::cout << "  Config generation failed" << std::endl;
        }
    }
    
    cm.stop();
    return 0;
}
