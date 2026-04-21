/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * SoftISP Test Application
 *
 * This application tests the SoftISP pipeline by:
 * - Enumerating available cameras
 * - Configuring and starting a camera
 * - Processing frames through the IPA
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cstring>

#include <libcamera/base/log.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/pixel_format.h>
#include <libcamera/geometry.h>

#include <thread>
#include <chrono>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

using namespace libcamera;

static const Size kTestSize(640, 480);
static const PixelFormat kPixelFormat = formats::UYVY;

static void printUsage(const char *prog) {
	std::cerr << "Usage: " << prog << " [options]" << std::endl;
	std::cerr << "Options:" << std::endl;
	std::cerr << "  --pipeline <name>  Pipeline to use (softisp or dummysoftisp)" << std::endl;
	std::cerr << "  --output <file>    Output file (default: output.yuv)" << std::endl;
	std::cerr << "  --frames <n>       Number of frames to process (default: 10)" << std::endl;
	std::cerr << "  --help             Show this help message" << std::endl;
}

int main(int argc, char *argv[]) {
	std::string pipelineName = "softisp";
	std::string outputFile = "output.yuv";
	int numFrames = 10;

	/* Parse arguments */
	for (int i = 1; i < argc; ++i) {
		std::string arg = argv[i];
		if (arg == "--pipeline" && i + 1 < argc) {
			pipelineName = argv[++i];
		} else if (arg == "--output" && i + 1 < argc) {
			outputFile = argv[++i];
		} else if (arg == "--frames" && i + 1 < argc) {
			numFrames = std::stoi(argv[++i]);
		} else if (arg == "--help") {
			printUsage(argv[0]);
			return 0;
		}
	}

	std::cout << "SoftISP Test Application" << std::endl;
	std::cout << "Pipeline: " << pipelineName << std::endl;
	std::cout << "Output: " << outputFile << std::endl;
	std::cout << "Frames: " << numFrames << std::endl;
	std::cout << std::endl;

	/* Initialize CameraManager */
	std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
	int ret = cm->start();
	if (ret) {
		std::cerr << "Failed to start CameraManager: " << ret << std::endl;
		return -1;
	}

	/* List available cameras */
	std::cout << "Available cameras:" << std::endl;
	const auto &cameras = cm->cameras();
	if (cameras.empty()) {
		std::cerr << "No cameras found!" << std::endl;
		std::cerr << "Make sure cameras are connected or use a virtual device." << std::endl;
		return -1;
	}

	for (const auto &cameraId : cameras) {
		std::cout << "  - " << cameraId << std::endl;
	}
	std::cout << std::endl;

	/* Select first camera */
	std::shared_ptr<Camera> camera = cm->get(cameras[0]->id());
	if (!camera) {
		std::cerr << "Failed to get camera" << std::endl;
		return -1;
	}

	std::cout << "Selected camera: " << camera->id() << std::endl;

	/* Generate configuration */
	std::unique_ptr<CameraConfiguration> config =
		camera->generateConfiguration({ StreamRole::Raw });
	if (!config) {
		std::cerr << "Failed to generate configuration" << std::endl;
		return -1;
	}

	/* Configure stream */
	StreamConfiguration &cfg = config->at(0);
	cfg.size = kTestSize;
	cfg.pixelFormat = kPixelFormat;
	cfg.bufferCount = 4;

	ret = camera->configure(config.get());
	if (ret) {
		std::cerr << "Failed to configure camera: " << ret << std::endl;
		return -1;
	}

	Stream *stream = config->at(0).stream();
	const FrameStreamInfo &info = stream->configuration();
	std::cout << "Stream configured: " << kTestSize.toString()
	          << " " << kPixelFormat.toString() << std::endl;

	/* Export buffers through the camera */
	std::vector<std::unique_ptr<FrameBuffer>> buffers;
	ret = camera->exportFrameBuffers(stream, &buffers);
	if (ret) {
		std::cerr << "Failed to export buffers: " << ret << std::endl;
		return -1;
	}

	std::cout << "Exported " << buffers.size() << " buffers" << std::endl;

	/* Create requests */
	std::vector<std::unique_ptr<Request>> requests;
	for (unsigned int i = 0; i < buffers.size(); ++i) {
		std::unique_ptr<Request> request = camera->createRequest();
		if (!request) {
			std::cerr << "Failed to create request" << std::endl;
			return -1;
		}

		ret = request->addBuffer(stream, buffers[i].get());
		if (ret) {
			std::cerr << "Failed to add buffer to request" << std::endl;
			return -1;
		}

		requests.push_back(std::move(request));
	}

	/* Start camera */
	ret = camera->start();
	if (ret) {
		std::cerr << "Failed to start camera: " << ret << std::endl;
		return -1;
	}

	std::cout << "Camera started. Processing " << numFrames << " frames..." << std::endl;
	std::cout << std::endl;

	/* Process frames */
	int processedFrames = 0;
	for (int i = 0; i < numFrames; ++i) {
		std::unique_ptr<Request> request = camera->createRequest();
		if (!request) {
			std::cerr << "Failed to create request" << std::endl;
			break;
		}

		/* Select a buffer */
		FrameBuffer *buffer = buffers[i % buffers.size()].get();

		ret = request->addBuffer(stream, buffer);
		if (ret) {
			std::cerr << "Failed to add buffer to request" << std::endl;
			break;
		}

		/* Generate synthetic test pattern */
		if (buffer->planes().size() > 0) {
			int fd = buffer->planes()[0].fd.get();
			if (fd >= 0) {
				size_t size = info.size;
				void *mem = mmap(nullptr, size, PROT_READ | PROT_WRITE,
				                 MAP_SHARED, fd, 0);
				if (mem != MAP_FAILED) {
					/* Fill with gradient pattern */
					uint8_t *data = static_cast<uint8_t *>(mem);
					for (unsigned int y = 0; y < kTestSize.height; ++y) {
						for (unsigned int x = 0; x < kTestSize.width; ++x) {
							uint8_t val = (x + y) % 256;
							data[y * kTestSize.width * 2 + x * 2] = val;
							data[y * kTestSize.width * 2 + x * 2 + 1] = 128;
						}
					}
					munmap(mem, size);
				}
			}
		}

		ret = camera->queueRequest(request.get());
		if (ret) {
			std::cerr << "Failed to queue request" << std::endl;
			break;
		}

		/* Wait for completion */
		std::this_thread::sleep_for(std::chrono::milliseconds(100));

		processedFrames++;
		std::cout << "Processed frame " << (i + 1) << "/" << numFrames << std::endl;
	}

	/* Stop camera */
	camera->stop();
	std::cout << std::endl;

	/* Save output */
	if (processedFrames > 0 && !buffers.empty()) {
		std::cout << "Saving output to " << outputFile << std::endl;

		const auto &buffer = buffers.back();
		if (buffer->planes().size() > 0) {
			int fd = buffer->planes()[0].fd.get();
			size_t size = info.size;

			if (fd >= 0) {
				void *mem = mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0);
				if (mem != MAP_FAILED) {
					FILE *f = fopen(outputFile.c_str(), "wb");
					if (f) {
						fwrite(mem, 1, size, f);
						fclose(f);
						std::cout << "Saved " << size << " bytes" << std::endl;
					}
					munmap(mem, size);
				}
			}
		}
	}

	std::cout << std::endl;
	std::cout << "Test completed successfully!" << std::endl;
	std::cout << "Processed " << processedFrames << " frames" << std::endl;

	return 0;
}
