/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

namespace libcamera {
namespace ipa {
namespace soft {

void SoftIsp::processFrame(const uint32_t frame, const uint32_t bufferId,
			   const SharedFD &bufferFd, const int32_t /*planeIndex*/,
			   const int32_t width, const int32_t height,
			   const ControlList &results)
{
	if (!impl_->initialized)
		return;

	// Calculate buffer size for SBGGR10 (10 bits per pixel, packed)
	size_t bufferSize = ((width * 10 + 7) / 8) * height;

	// Map the buffer for reading/writing
	void *bayerData = mmap(nullptr, bufferSize, PROT_READ | PROT_WRITE,
	                       MAP_SHARED, bufferFd.get(), 0);
	if (bayerData == MAP_FAILED) {
		frameDone.emit(frame, bufferId);
		return;
	}

	// TODO: Read AWB/AE parameters from 'results' ControlList
	float redGain = 1.0f;
	float blueGain = 1.0f;
	
	// Simplified: just use default gains for now
	
	// Convert SBGGR10 Bayer to float array for ONNX input
	std::vector<float> bayerFloat;
	bayerFloat.reserve(width * height);
	
	uint8_t *byteData = static_cast<uint8_t*>(bayerData);
	for (int32_t i = 0; i < width * height; i++) {
		float pixel = static_cast<float>(byteData[i % bufferSize]) / 255.0f;
		bayerFloat.push_back(pixel);
	}

	// Apply AWB gains to Bayer data (pre-processing)
	for (int32_t j = 0; j < width * height; j++) {
		int32_t row = j / width;
		int32_t col = j % width;
		
		bool evenRow = (row % 2 == 0);
		bool evenCol = (col % 2 == 0);
		
		if (evenRow && !evenCol) {
			// B pixel - apply blue gain
			bayerFloat[j] *= blueGain;
		} else if (!evenRow && evenCol) {
			// R pixel - apply red gain
			bayerFloat[j] *= redGain;
		}
	}

	// Run applier.onnx inference (Bayer → RGB/YUV)
	std::vector<float> rgbOutput;
	int ret = impl_->applierEngine.runInference(bayerFloat, rgbOutput);
	if (ret < 0) {
		munmap(bayerData, bufferSize);
		frameDone.emit(frame, bufferId);
		return;
	}

	// Write RGB output back to buffer
	size_t outputSize = std::min(rgbOutput.size(), static_cast<size_t>(width * height * 3));
	for (size_t i = 0; i < outputSize && i < bufferSize; i++) {
		byteData[i] = static_cast<uint8_t>(rgbOutput[i] * 255.0f);
	}

	// Unmap buffer
	munmap(bayerData, bufferSize);

	// Signal completion
	frameDone.emit(frame, bufferId);
}

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */
