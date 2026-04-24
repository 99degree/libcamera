/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * softisp-save.cpp - SoftISP Virtual Camera Frame Capture Utility
 * 
 * Note: This is a simplified example. For production use, use the 'cam' tool.
 */

#include <iostream>
#include <memory>
#include <signal.h>
#include <string>
#include <thread>

#include <libcamera/camera_manager.h>
#include <libcamera/camera.h>

using namespace libcamera;

static bool running = true;

static void signalHandler(int)
{
    running = false;
}

int main(int argc, char *argv[])
{
    std::string cameraId = "softisp_virtual";
    
    if (argc > 1 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "Usage: softisp-save [camera_id]\n";
        std::cout << "  camera_id: Camera ID (default: softisp_virtual)\n";
        return 0;
    }
    
    if (argc > 1) {
        cameraId = argv[1];
    }

    std::cout << "SoftISP Frame Capture Utility (Demo)\n";
    std::cout << "Camera: " << cameraId << "\n\n";

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    CameraManager cameraManager;
    int ret = cameraManager.start();
    if (ret < 0) {
        std::cerr << "Failed to start CameraManager\n";
        return -1;
    }

    std::shared_ptr<Camera> cam = cameraManager.get(cameraId);
    if (!cam) {
        std::cerr << "Camera not found: " << cameraId << "\n";
        cameraManager.stop();
        return -1;
    }

    std::cout << "Camera found: " << cameraId << "\n";
    std::cout << "Camera is ready for capture!\n";
    std::cout << "\nUse 'cam -c " << cameraId << " --capture' to capture frames.\n";
    std::cout << "Or use 'cam -c " << cameraId << " --info' to see camera info.\n";

    // Keep running until interrupted
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    cam->stop();
    cameraManager.stop();
    
    std::cout << "\nShutdown complete.\n";
    return 0;
}
