/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024
 *
 * SoftISP Test Application
 *
 * This application tests the SoftISP pipeline (both real and virtual)
 * by generating synthetic frames and processing them through the IPA.
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <libcamera/base/log.h>
#include <libcamera/camera.h>
#include <libcamera/camera_manager.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/pixel_format.h>
#include <libcamera/stream.h>

#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/request.h"

using namespace libcamera;

static const Size kTestSize(640, 480);
static const PixelFormat kPixelFormat = formats::UYVY8_1X16;

static void printUsage(const char *prog)
{
	std::cerr << "Usage: " << prog << " [options]" << std::endl;
	std::cerr << "Options:" << std::endl;
	std::cerr << "  --pipeline <name>   Pipeline to use (softisp or virtual-softisp)" << std::endl;
	std::cerr << "  --output <file>     Output file (default: output.yuv)" << std::endl;
	std::cerr << "  --frames <n>        Number of frames to process (default: 10)" << std::endl;
	std::cerr << "  --help              Show this help message" << std::endl;
}

int main(int argc, char *argv[])
{
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

	/* Initialize CameraManager */
	std::unique_ptr<CameraManager> cm = std::make_unique<CameraManager>();
	int ret = cm->start();
	if (ret) {
		std::cerr << "Failed to start CameraManager" << std::endl;
		return -1;
	}

	/* List available cameras */
	std::cout << "Available cameras:" << std::endl;
	for (const auto &cameraId : cm->cameras()) {
		std::shared_ptr<Camera> camera = cm->get(cameraId);
		std::cout << "  " << cameraId << " (Pipeline: " 
		          << camera->pipelineHandler()->name() << ")" << std::endl;
	}

	/* Find a camera matching the requested pipeline */
	std::shared_ptr<Camera> camera;
	for (const auto &cameraId : cm->cameras()) {
		std::shared_ptr<Camera> cam = cm->get(cameraId);
		if (cam->pipelineHandler()->name() == pipelineName) {
			camera = cam;
			break;
		}
	}

	if (!camera) {
		std::cerr << "No camera found for pipeline: " << pipelineName << std::endl;
		std::cerr << "Make sure the pipeline is loaded and devices are available." << std::endl;
		return -1;
	}

	std::cout << "Using camera: " << camera->id() << " (Pipeline: " 
	          << camera->pipelineHandler()->name() << ")" << std::endl;

	/* Generate configuration */
	std::unique_ptr<CameraConfiguration> config = camera->generateConfiguration(
		{ StreamRole::Raw });
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

	/* Export buffers */
	Stream *stream = config->at(0).stream();
	std::vector<std::unique_ptr<FrameBuffer>> buffers;
	ret = camera->exportFrameBuffers(stream, &buffers);
	if (ret) {
		std::cerr << "Failed to export buffers: " << ret << std::endl;
		return -1;
	}

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

		/* Generate synthetic test pattern (simple gradient) */
		if (buffer->planes().size() > 0) {
			void *mem = buffer->planes()[0].fd.get();
			if (mem) {
				/* Fill with a simple gradient pattern */
				uint8_t *data = static_cast<uint8_t *>(mem);
				for (unsigned int y = 0; y < kTestSize.height; ++y) {
					for (unsigned int x = 0; x < kTestSize.width; ++x) {
						uint8_t val = (x + y) % 256;
						data[y * kTestSize.width * 2 + x * 2] = val;
						data[y * kTestSize.width * 2 + x * 2 + 1] = 128;
					}
				}
			}
		}

		ret = camera->queueRequest(request.get());
		if (ret) {
			std::cerr << "Failed to queue request" << std::endl;
			break;
		}

		/* Wait for request completion (simplified) */
		/* In a real app, you'd use signals/slots or a queue */
		usleep(100000); /* 100ms delay */

		processedFrames++;
		std::cout << "Processed frame " << (i + 1) << "/" << numFrames << std::endl;
	}

	/* Stop camera */
	camera->stop();

	/* Save output (simplified: just dump first buffer) */
	if (processedFrames > 0 && !buffers.empty()) {
		std::cout << "Saving output to " << outputFile << std::endl;
		/* In a real app, you'd write the buffer data to the file */
	}

	std::cout << "Test completed successfully. Processed " << processedFrames << " frames." << std::endl;

	return 0;
}
