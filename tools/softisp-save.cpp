/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * SoftISP Frame Saver
 * A minimal tool to capture and save frames from the SoftISP pipeline.
 * Bypasses the complex file logic of libcamera-cam to avoid crashes.
 */

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
#include <libcamera/formats.h>
#include <libcamera/framebuffer_allocator.h>
#include <libcamera/request.h>
#include <libcamera/stream.h>

using namespace libcamera;

LOG_DEFINE_CATEGORY(SoftIspSave)

static int saveFrame(const FrameBuffer *buffer, const std::string &filename)
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
	return 0;
}

int main(int argc, char *argv[])
{
	int frames = 5;
	std::string cameraId = "SoftISP";
	std::string outputDir = ".";

	static struct option long_options[] = {
		{ "frames", required_argument, 0, 'f' },
		{ "camera", required_argument, 0, 'c' },
		{ "output", required_argument, 0, 'o' },
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	int opt;
	while ((opt = getopt_long(argc, argv, "f:c:o:h", long_options, nullptr)) != -1) {
		switch (opt) {
		case 'f': frames = std::stoi(optarg); break;
		case 'c': cameraId = optarg; break;
		case 'o': outputDir = optarg; break;
		case 'h':
		default:
			std::cout << "Usage: " << argv[0] << " [options]\n"
			          << "Options:\n"
			          << "  -f, --frames N   Number of frames (default: 5)\n"
			          << "  -c, --camera ID  Camera ID substring (default: SoftISP)\n"
			          << "  -o, --output DIR Output directory (default: .)\n"
			          << "  -h, --help       Show this help\n";
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
		return -1;
	}
	std::cout << "Using camera: " << camera->id() << std::endl;

	auto config = camera->generateConfiguration({ StreamRole::Raw });
	if (!config) {
		std::cerr << "Failed to generate configuration" << std::endl;
		return -1;
	}

	if (config->validate() == CameraConfiguration::Invalid) {
		std::cerr << "Invalid configuration" << std::endl;
		return -1;
	}

	if (camera->configure(config.get()) < 0) {
		std::cerr << "Failed to configure camera" << std::endl;
		return -1;
	}

	Stream *stream = config->at(0).stream();
	if (!stream) {
		std::cerr << "No stream found" << std::endl;
		return -1;
	}

	FrameBufferAllocator allocator(camera);
	if (allocator.allocate(stream) < 0) {
		std::cerr << "Failed to allocate buffers" << std::endl;
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
			return -1;
		}
		if (request->addBuffer(stream, buffer) < 0) {
			std::cerr << "Failed to add buffer to request" << std::endl;
			return -1;
		}
		requests.push_back(std::move(request));
	}

	if (camera->start() < 0) {
		std::cerr << "Failed to start camera" << std::endl;
		return -1;
	}

	std::cout << "Capturing " << frames << " frames..." << std::endl;

	int saved = 0;
	for (int i = 0; i < frames; ++i) {
		Request &request = *requests[i % requests.size()];
		
		if (camera->queueRequest(&request) < 0) {
			std::cerr << "Failed to queue request" << std::endl;
			break;
		}

		// Wait for completion (simple polling)
		while (request.status() == Request::RequestPending) {
			usleep(10000);
		}

		if (request.status() == Request::RequestComplete) {
			const auto &reqBuffers = request.buffers();
			if (!reqBuffers.empty()) {
				FrameBuffer *buffer = reqBuffers.begin()->second;
				std::string filename = outputDir + "/frame_" + std::to_string(i) + ".bin";
				if (saveFrame(buffer, filename) == 0) {
					saved++;
				}
			}
			request.reuse();
		} else {
			std::cerr << "Request failed: " << request.status() << std::endl;
		}
	}

	camera->stop();
	cm.stop();

	std::cout << "Completed: Saved " << saved << "/" << frames << " frames." << std::endl;
	return (saved > 0) ? 0 : -1;
}
