/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * SoftISP Frame Saver
 * A minimal tool to capture and save frames from the SoftISP pipeline.
 * Supports Virtual Camera, metadata export, and continuous mode.
 */
#include <csignal>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <libcamera/base/log.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/json.h>
#include <libcamera/property_ids.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>

using namespace libcamera;

LOG_DEFINE_CATEGORY(SoftIspSave)

static int saveFrame(const FrameBuffer *buffer, const std::string &filename, const ControlList *metadata = nullptr)
{
    const auto &planes = buffer->planes();
    if (planes.empty()) {
        std::cerr << "No planes in buffer" << std::endl;
        return -1;
    }

    const FrameBuffer::Plane &plane = planes[0];
    int fd = plane.fd.get();
    if (fd < 0) {
        std::cerr << "Invalid file descriptor" << std::endl;
        return -1;
    }

    // Map the buffer
    void *memory = mmap(nullptr, plane.length, PROT_READ, MAP_SHARED, fd, 0);
    if (memory == MAP_FAILED) {
        std::cerr << "Failed to mmap buffer: " << strerror(errno) << std::endl;
        return -1;
    }

    // Write to file
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        munmap(memory, plane.length);
        return -1;
    }
    out.write(static_cast<char*>(memory), plane.length);
    out.close();

    munmap(memory, plane.length);

    std::cout << "Saved " << plane.length << " bytes to " << filename << std::endl;

    // Save metadata if provided
    if (metadata && !metadata->empty()) {
        std::string metaFilename = filename.substr(0, filename.find_last_of('.')) + ".json";
        std::ofstream metaOut(metaFilename);
        if (metaOut) {
            Json::Value json;
            for (const auto &[id, ctrl] : *metadata) {
                std::string key = "control_" + std::to_string(id);
                if (ctrl.type() == ControlTypeFloat) {
                    json[key] = ctrl.to<float>();
                } else if (ctrl.type() == ControlTypeInteger) {
                    json[key] = ctrl.to<int32_t>();
                }
                // Skip binary data for now
            }
            metaOut << Json::toString(json) << std::endl;
            std::cout << "Saved metadata to " << metaFilename << std::endl;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int frames = 5;
    std::string cameraId = "SoftISP";
    std::string outputDir = ".";
    bool continuous = false;
    bool saveMetadata = true;
    bool detectFormat = true;

    static struct option long_options[] = {
        { "frames",      required_argument, 0, 'f' },
        { "camera",      required_argument, 0, 'c' },
        { "output",      required_argument, 0, 'o' },
        { "continuous",  no_argument,       0, 'C' },
        { "no-metadata", no_argument,       0, 'M' },
        { "help",        no_argument,       0, 'h' },
        { 0, 0, 0, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "f:c:o:CMh", long_options, nullptr)) != -1) {
        switch (opt) {
        case 'f':
            frames = std::stoi(optarg);
            break;
        case 'c':
            cameraId = optarg;
            break;
        case 'o':
            outputDir = optarg;
            break;
        case 'C':
            continuous = true;
            break;
        case 'M':
            saveMetadata = false;
            break;
        case 'h':
        default:
            std::cout << "Usage: " << argv[0] << " [options]\n"
                      << "Options:\n"
                      << " -f, --frames N     Number of frames (default: 5, 0 for continuous)\n"
                      << " -c, --camera ID    Camera ID substring (default: SoftISP)\n"
                      << " -o, --output DIR   Output directory (default: .)\n"
                      << " -C, --continuous   Continuous mode (Ctrl+C to stop)\n"
                      << " -M, --no-metadata  Disable metadata saving\n"
                      << " -h, --help         Show this help\n";
            return 0;
        }
    }

    // Create output directory if needed
    if (outputDir != "." && mkdir(outputDir.c_str(), 0755) != 0 && errno != EEXIST) {
        std::cerr << "Failed to create output directory: " << outputDir << std::endl;
        return -1;
    }

    CameraManager cm;
    if (cm.start() < 0) {
        std::cerr << "Failed to start CameraManager" << std::endl;
        return -1;
    }

    auto cameras = cm.cameras();
    std::shared_ptr<Camera> camera;
    for (auto &cam : cameras) {
        if (cam->id().find(cameraId) != std::string::npos) {
            camera = cam;
            break;
        }
    }

    if (!camera) {
        std::cerr << "Camera not found: " << cameraId << std::endl;
        cm.stop();
        return -1;
    }

    if (camera->acquire() < 0) {
        std::cerr << "Failed to acquire camera" << std::endl;
        cm.stop();
        return -1;
    }

    std::cout << "Using camera: " << camera->id() << std::endl;

    // Generate configuration
    auto config = camera->generateConfiguration({ StreamRole::Raw });
    if (!config) {
        std::cerr << "Failed to generate configuration" << std::endl;
        camera->release();
        cm.stop();
        return -1;
    }

    if (config->validate() == CameraConfiguration::Invalid) {
        std::cerr << "Invalid configuration" << std::endl;
        camera->release();
        cm.stop();
        return -1;
    }

    // Try to detect format
    if (detectFormat) {
        const Stream *stream = config->at(0).stream();
        if (stream) {
            std::cout << "Stream format: " << stream->format().toString() << std::endl;
            std::cout << "Stream size: " << stream->size().width << "x" << stream->size().height << std::endl;
        }
    }

    if (camera->configure(config.get()) < 0) {
        std::cerr << "Failed to configure camera" << std::endl;
        camera->release();
        cm.stop();
        return -1;
    }

    Stream *stream = config->at(0).stream();
    if (!stream) {
        std::cerr << "No stream found" << std::endl;
        camera->release();
        cm.stop();
        return -1;
    }

    FrameBufferAllocator allocator(camera);
    if (allocator.allocate(stream) < 0) {
        std::cerr << "Failed to allocate buffers" << std::endl;
        camera->release();
        cm.stop();
        return -1;
    }

    const auto &allocatedBuffers = allocator.buffers(stream);
    std::vector<FrameBuffer*> buffers;
    for (auto &buf : allocatedBuffers) {
        buffers.push_back(buf.get());
    }

    std::vector<std::unique_ptr<Request>> requests;
    for (auto &buffer : buffers) {
        std::unique_ptr<Request> request = camera->createRequest();
        if (!request) {
            std::cerr << "Failed to create request" << std::endl;
            camera->release();
            cm.stop();
            return -1;
        }
        if (request->addBuffer(stream, buffer) < 0) {
            std::cerr << "Failed to add buffer to request" << std::endl;
            camera->release();
            cm.stop();
            return -1;
        }
        requests.push_back(std::move(request));
    }

    if (camera->start() < 0) {
        std::cerr << "Failed to start camera" << std::endl;
        camera->release();
        cm.stop();
        return -1;
    }

    std::cout << "Capturing frames..." << (continuous ? " (Continuous mode, press Ctrl+C to stop)" : "") << std::endl;

    int saved = 0;
    int frameCount = 0;
    bool running = true;

    // Signal handler for continuous mode
    auto signalHandler = [](int sig) {
        std::cout << "\nStopping capture..." << std::endl;
        running = false;
    };
    signal(SIGINT, signalHandler);

    while (running && (frames == 0 || frameCount < frames)) {
        Request &request = *requests[frameCount % requests.size()];
        if (camera->queueRequest(&request) < 0) {
            std::cerr << "Failed to queue request" << std::endl;
            break;
        }

        // Wait for completion
        while (request.status() == Request::RequestPending) {
            usleep(10000);
            if (!running) break;
        }

        if (!running) break;

        if (request.status() == Request::RequestComplete) {
            const auto &reqBuffers = request.buffers();
            if (!reqBuffers.empty()) {
                FrameBuffer *buffer = reqBuffers.begin()->second;
                std::string filename = outputDir + "/frame_" + std::to_string(frameCount) + ".bin";
                
                const ControlList *metadata = nullptr;
                if (saveMetadata) {
                    metadata = &request.metadata();
                }

                if (saveFrame(buffer, filename, metadata) == 0) {
                    saved++;
                    frameCount++;
                    std::cout << "Captured frame " << frameCount << (frames > 0 ? ("/" + std::to_string(frames)) : "") << std::endl;
                }
            }
            request.reuse();
        } else {
            std::cerr << "Request failed: " << request.status() << std::endl;
            frameCount++;
        }
    }

    camera->stop();
    camera->release();
    cm.stop();

    std::cout << "Completed: Saved " << saved << " frames." << std::endl;
    return (saved > 0) ? 0 : -1;
}
