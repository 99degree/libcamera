/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

void SoftIsp::processFrame(const uint32_t frame, const uint32_t bufferId,
			   const libcamera::SharedFD &bufferFd,
			   const int32_t /*planeIndex*/,
			   const int32_t width, const int32_t height,
			   const libcamera::ControlList &results)
{
	if (!impl_->initialized)
		return;

	LOG(IPASoftISP, Debug) << "processFrame: frame=" << frame << " size=" << width << "x" << height;

	// Calculate buffer size for SBGGR10 (10 bits per pixel, packed)
	size_t bufferSize = ((width * 10 + 7) / 8) * height;

	// Map the buffer for reading/writing
	void *bayerData = mmap(nullptr, bufferSize, PROT_READ | PROT_WRITE,
			       MAP_SHARED, bufferFd.get(), 0);
	if (bayerData == MAP_FAILED) {
		LOG(IPASoftISP, Error) << "Failed to map buffer";
		frameDone.emit(frame, bufferId);
		return;
	}

	// Convert Bayer data to float array for ONNX input
	std::vector<float> bayerFloat;
	bayerFloat.reserve(width * height);

	uint8_t *byteData = static_cast<uint8_t*>(bayerData);
	for (size_t i = 0; i < width * height; i++) {
		float pixel = static_cast<float>(byteData[i % bufferSize]) / 255.0f;
		bayerFloat.push_back(pixel);
	}

	// Run applier.onnx inference (Bayer → RGB/YUV)
	std::vector<float> rgbOutput;
	int ret = impl_->applierEngine.runInference(bayerFloat, rgbOutput);
	if (ret < 0) {
		LOG(IPASoftISP, Error) << "applierEngine inference failed: " << ret;
		munmap(bayerData, bufferSize);
		frameDone.emit(frame, bufferId);
		return;
	}

	// Write RGB output back to buffer
	size_t outputSize = std::min(rgbOutput.size(), static_cast<size_t>(width * height * 3));
	for (size_t i = 0; i < outputSize && i < bufferSize; i++) {
		byteData[i] = static_cast<uint8_t>(std::min(255.0f, std::max(0.0f, rgbOutput[i] * 255.0f)));
	}

	// Unmap buffer
	munmap(bayerData, bufferSize);

	// Emit frame done signal
	frameDone.emit(frame, bufferId);

	LOG(IPASoftISP, Debug) << "Frame processed successfully";
}
