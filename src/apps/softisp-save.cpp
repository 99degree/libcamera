/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * softisp-save.cpp - Advanced SoftISP Frame Capture Utility
 * Saves frames in multiple formats (YUV, RGB) with metadata
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iomanip>
#include <ctime>

#include <libcamera/libcamera.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>

using namespace libcamera;

static bool g_running = true;

void signalHandler(int signum) {
    g_running = false;
}

enum class OutputFormat {
    RAW,    // Raw Bayer data
    YUV,    // YUV420/YUV422
    RGB,    // RGB888
    PPM     // Portable Pixmap format
};

std::string formatToString(OutputFormat format) {
    switch (format) {
        case OutputFormat::RAW: return "RAW";
        case OutputFormat::YUV: return "YUV";
        case OutputFormat::RGB: return "RGB";
        case OutputFormat::PPM: return "PPM";
        default: return "UNKNOWN";
    }
}

OutputFormat parseFormat(const std::string& str) {
    if (str == "raw" || str == "bayer") return OutputFormat::RAW;
    if (str == "yuv" || str == "yuv420" || str == "yuv422") return OutputFormat::YUV;
    if (str == "rgb" || str == "rgb888") return OutputFormat::RGB;
    if (str == "ppm") return OutputFormat::PPM;
    return OutputFormat::RAW; // Default
}

void saveMetadata(const ControlList& controls, const std::string& filename, int frameNum) {
    std::ofstream metaFile(filename + ".meta");
    if (!metaFile) {
        std::cerr << "Warning: Could not create metadata file" << std::endl;
        return;
    }
    
    metaFile << "Frame: " << frameNum << "\n";
    metaFile << "Timestamp: " << std::time(nullptr) << "\n";
    metaFile << "Controls:\n";
    
    for (const auto& [id, control] : controls) {
        metaFile << "  " << id << ": " << control.toString() << "\n";
    }
    
    metaFile.close();
    std::cout << "  Metadata saved to: " << filename << ".meta" << std::endl;
}

void convertBayerToYUV(uint8_t* bayerData, uint8_t* yuvData, int width, int height) {
    // Simple Bayer to YUV conversion (for testing)
    // In production, this would use the actual ISP pipeline
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 2; // 10-bit packed
            uint16_t pixel = (bayerData[idx] | (bayerData[idx + 1] << 8)) & 0x3FF;
            
            // Simple grayscale conversion
            uint8_t y = (pixel * 255) / 1023;
            uint8_t u = 128;
            uint8_t v = 128;
            
            // Store in YUV420 planar format
            if (y < height && x < width) {
                yuvData[y * width + x] = y;
            }
            if (y % 2 == 0 && x % 2 == 0) {
                int uvIdx = (height * width) + (y / 2 * width / 2) + (x / 2);
                yuvData[uvIdx] = u;
                yuvData[uvIdx + (height * width) / 4] = v;
            }
        }
    }
}

void convertBayerToRGB(uint8_t* bayerData, uint8_t* rgbData, int width, int height) {
    // Simple Bayer to RGB conversion
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 2;
            uint16_t pixel = (bayerData[idx] | (bayerData[idx + 1] << 8)) & 0x3FF;
            
            // Simple color mapping based on RGGB pattern
            uint8_t r, g, b;
            if ((x + y) % 2 == 0) {
                // Red or Green pixel
                r = (pixel * 255) / 1023;
                g = (pixel * 255) / 1023;
                b = 0;
            } else {
                // Green or Blue pixel
                r = 0;
                g = (pixel * 255) / 1023;
                b = (pixel * 255) / 1023;
            }
            
            int rgbIdx = (y * width + x) * 3;
            rgbData[rgbIdx] = r;
            rgbData[rgbIdx + 1] = g;
            rgbData[rgbIdx + 2] = b;
        }
    }
}

void saveAsPPM(std::ofstream& file, uint8_t* rgbData, int width, int height) {
    file << "P6\n" << width << " " << height << "\n255\n";
    file.write(reinterpret_cast<char*>(rgbData), width * height * 3);
}

void printUsage(const char* prog) {
    std::cout << "Usage: " << prog << " [options]\n\n"
              << "Advanced SoftISP frame capture with multiple output formats\n\n"
              << "Options:\n"
              << "  -c, --camera <id>        Camera ID (default: softisp_virtual)\n"
              << "  -o, --output <file>      Output file pattern (default: frame-#.raw)\n"
              << "                           Use # for frame number\n"
              << "  -f, --format <fmt>       Output format: raw, yuv, rgb, ppm (default: raw)\n"
              << "  -n, --frames <num>       Number of frames to capture (default: 1)\n"
              << "  -w, --width <width>      Image width (default: 1920)\n"
              << "  -H, --height <height>    Image height (default: 1080)\n"
              << "  -m, --metadata           Save metadata to .meta files\n"
              << "  -v, --verbose            Verbose output\n"
              << "  -h, --help               Show this help\n\n"
              << "Examples:\n"
              << "  " << prog << " -c softisp_virtual -n 5 -f yuv -o capture-#\n"
              << "  " << prog << " -f rgb --metadata -n 3 -o frame-#.ppm\n";
}

int main(int argc, char *argv[]) {
    // Default parameters
    std::string cameraId = "softisp_virtual";
    std::string outputPattern = "frame-#.raw";
    OutputFormat outputFormat = OutputFormat::RAW;
    int numFrames = 1;
    int width = 1920;
    int height = 1080;
    bool saveMetadata = false;
    bool verbose = false;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-c" || arg == "--camera") && i + 1 < argc) {
            cameraId = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputPattern = argv[++i];
        } else if ((arg == "-f" || arg == "--format") && i + 1 < argc) {
            outputFormat = parseFormat(argv[++i]);
        } else if ((arg == "-n" || arg == "--frames") && i + 1 < argc) {
            numFrames = std::atoi(argv[++i]);
        } else if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            width = std::atoi(argv[++i]);
        } else if ((arg == "-H" || arg == "--height") && i + 1 < argc) {
            height = std::atoi(argv[++i]);
        } else if (arg == "-m" || arg == "--metadata") {
            saveMetadata = true;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
    }
    
    std::cout << "=== SoftISP Advanced Frame Capture ===" << std::endl;
    std::cout << "Camera: " << cameraId << std::endl;
    std::cout << "Output: " << outputPattern << std::endl;
    std::cout << "Format: " << formatToString(outputFormat) << std::endl;
    std::cout << "Resolution: " << width << "x" << height << std::endl;
    std::cout << "Frames: " << numFrames << std::endl;
    std::cout << "Metadata: " << (saveMetadata ? "Enabled" : "Disabled") << std::endl;
    std::cout << std::endl;
    
    // Setup signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    // Initialize CameraManager
    if (verbose) std::cout << "Initializing CameraManager..." << std::endl;
    std::unique_ptr<CameraManager> cameraManager = std::make_unique<CameraManager>();
    cameraManager->start();
    
    // Find camera
    if (verbose) std::cout << "Searching for camera: " << cameraId << std::endl;
    const CameraIdArray& cameras = cameraManager->cameras();
    const Camera* targetCamera = nullptr;
    
    for (const auto& cid : cameras) {
        std::unique_ptr<Camera> camera = cameraManager->get(cid);
        std::string id = cid.toString();
        if (verbose) std::cout << "  Found: " << id << std::endl;
        
        if (id.find(cameraId) != std::string::npos || cameraId == "softisp_virtual") {
            targetCamera = camera.release();
            break;
        }
    }
    
    if (!targetCamera) {
        std::cerr << "Error: Camera not found: " << cameraId << std::endl;
        std::cerr << "Note: This tool requires a running SoftISP virtual camera pipeline." << std::endl;
        std::cerr << "The virtual camera may not be available on Termux/Android without V4L2 support." << std::endl;
        cameraManager->stop();
        return -1;
    }
    
    std::cout << "Camera found: " << targetCamera->id() << std::endl;
    
    // Acquire camera
    if (verbose) std::cout << "Acquiring camera..." << std::endl;
    int ret = targetCamera->acquire();
    if (ret < 0) {
        std::cerr << "Error: Failed to acquire camera" << std::endl;
        cameraManager->stop();
        return -1;
    }
    
    // Generate configuration
    if (verbose) std::cout << "Generating configuration..." << std::endl;
    std::unique_ptr<CameraConfiguration> config = targetCamera->generateConfiguration({ StreamRole::Viewfinder });
    if (!config) {
        std::cerr << "Error: Failed to generate configuration" << std::endl;
        targetCamera->release();
        cameraManager->stop();
        return -1;
    }
    
    config->at(0).size = Size(width, height);
    
    // Set pixel format based on output format
    switch (outputFormat) {
        case OutputFormat::RAW:
            config->at(0).pixelFormat = formats::SBGGR10;
            break;
        case OutputFormat::YUV:
            config->at(0).pixelFormat = formats::NV12;
            break;
        case OutputFormat::RGB:
            config->at(0).pixelFormat = formats::RGB888;
            break;
        case OutputFormat::PPM:
            config->at(0).pixelFormat = formats::RGB888;
            break;
    }
    
    config->at(0).bufferCount = 4;
    
    ret = config->validate();
    if (ret == CameraConfiguration::Invalid) {
        std::cerr << "Error: Invalid configuration" << std::endl;
        targetCamera->release();
        cameraManager->stop();
        return -1;
    }
    
    targetCamera->configure(config.get());
    std::cout << "Configuration validated" << std::endl;
    
    // Allocate buffers
    if (verbose) std::cout << "Allocating buffers..." << std::endl;
    FrameBufferAllocator allocator(targetCamera);
    
    Stream* stream = config->at(0).stream();
    for (unsigned int i = 0; i < config->at(0).bufferCount; i++) {
        std::unique_ptr<FrameBuffer> buffer;
        ret = allocator.allocate(stream, &buffer);
        if (ret < 0) {
            std::cerr << "Error: Failed to allocate buffer" << std::endl;
            targetCamera->release();
            cameraManager->stop();
            return -1;
        }
    }
    
    std::cout << "Allocated " << allocator.buffers(stream).size() << " buffers" << std::endl;
    
    // Create request
    if (verbose) std::cout << "Creating request..." << std::endl;
    std::unique_ptr<Request> request = targetCamera->createRequest();
    if (!request) {
        std::cerr << "Error: Failed to create request" << std::endl;
        targetCamera->release();
        cameraManager->stop();
        return -1;
    }
    
    // Start camera
    if (verbose) std::cout << "Starting camera..." << std::endl;
    ret = targetCamera->start();
    if (ret < 0) {
        std::cerr << "Error: Failed to start camera" << std::endl;
        targetCamera->release();
        cameraManager->stop();
        return -1;
    }
    
    std::cout << "Capturing " << numFrames << " frame(s)..." << std::endl;
    
    // Capture frames
    for (int frame = 0; frame < numFrames && g_running; frame++) {
        const std::vector<std::unique_ptr<FrameBuffer>> &buffers = allocator.buffers(stream);
        if (buffers.empty()) {
            std::cerr << "Error: No buffers available" << std::endl;
            break;
        }
        
        ret = request->addBuffer(stream, buffers[0].get());
        if (ret < 0) {
            std::cerr << "Error: Failed to add buffer to request" << std::endl;
            break;
        }
        
        ret = targetCamera->queueRequest(request.get());
        if (ret < 0) {
            std::cerr << "Error: Failed to queue request" << std::endl;
            break;
        }
        
        // Wait for completion
        usleep(100000); // 100ms
        
        std::cout << "  Frame " << (frame + 1) << " captured" << std::endl;
        
        // Generate output filename
        std::string filename = outputPattern;
        size_t pos = filename.find('#');
        if (pos != std::string::npos) {
            std::ostringstream numStr;
            numStr << std::setw(4) << std::setfill('0') << frame;
            filename.replace(pos, 1, numStr.str());
        }
        
        // Get buffer data
        const FrameBuffer::Plane &plane = buffers[0]->planes()[0];
        void *mem = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, plane.fd.get(), 0);
        if (mem == MAP_FAILED) {
            std::cerr << "Error: Failed to map buffer" << std::endl;
            break;
        }
        
        uint8_t* data = static_cast<uint8_t*>(mem);
        size_t dataSize = plane.length;
        
        // Save based on format
        if (outputFormat == OutputFormat::RAW) {
            // Save raw data directly
            std::ofstream outFile(filename, std::ios::binary);
            if (outFile) {
                outFile.write(reinterpret_cast<char*>(data), dataSize);
                outFile.close();
                std::cout << "  Saved: " << filename << " (" << dataSize << " bytes)" << std::endl;
            } else {
                std::cerr << "  Error: Could not open " << filename << std::endl;
            }
        } else if (outputFormat == OutputFormat::PPM) {
            // Save as PPM
            std::ofstream outFile(filename);
            if (outFile) {
                saveAsPPM(outFile, data, width, height);
                outFile.close();
                std::cout << "  Saved: " << filename << " (PPM format)" << std::endl;
            } else {
                std::cerr << "  Error: Could not open " << filename << std::endl;
            }
        } else if (outputFormat == OutputFormat::YUV || outputFormat == OutputFormat::RGB) {
            // Convert and save
            size_t convertedSize = (outputFormat == OutputFormat::YUV) ? 
                (width * height * 3 / 2) : (width * height * 3);
            
            std::vector<uint8_t> convertedData(convertedSize);
            
            if (outputFormat == OutputFormat::YUV) {
                convertBayerToYUV(data, convertedData.data(), width, height);
            } else {
                convertBayerToRGB(data, convertedData.data(), width, height);
            }
            
            std::ofstream outFile(filename, std::ios::binary);
            if (outFile) {
                outFile.write(reinterpret_cast<char*>(convertedData.data()), convertedSize);
                outFile.close();
                std::cout << "  Saved: " << filename << " (" << formatToString(outputFormat) 
                         << ", " << convertedSize << " bytes)" << std::endl;
            } else {
                std::cerr << "  Error: Could not open " << filename << std::endl;
            }
        }
        
        // Save metadata if requested
        if (saveMetadata) {
            // Get request controls (metadata)
            const ControlList& controls = request->controls();
            saveMetadata(controls, filename, frame);
        }
        
        munmap(mem, plane.length);
        
        // Create new request for next frame
        request = targetCamera->createRequest();
        if (!request) {
            std::cerr << "Error: Failed to create new request" << std::endl;
            break;
        }
    }
    
    // Stop camera
    std::cout << "Stopping camera..." << std::endl;
    targetCamera->stop();
    
    // Cleanup
    std::cout << "Cleaning up..." << std::endl;
    targetCamera->release();
    cameraManager->stop();
    
    std::cout << "\n🎉 Capture complete!" << std::endl;
    return 0;
}
