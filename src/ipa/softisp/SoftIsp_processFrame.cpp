/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstring>
#include <sys/mman.h>

void SoftIsp::processFrame(const uint32_t frame, const uint32_t bufferId,
			   const SharedFD &bufferFd, const int32_t /*planeIndex*/,
			   const int32_t width, const int32_t height,
			   const ControlList &results)
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "Not initialized";
		return;
	}

	LOG(SoftIsp, Debug) << "processFrame: frame=" << frame 
	                    << ", buffer=" << bufferId
	                    << ", size=" << width << "x" << height;

	// Calculate buffer size for SBGGR10 (10 bits per pixel, packed)
	size_t bufferSize = ((width * 10 + 7) / 8) * height;

	// Map the buffer for reading Bayer data
	void *bayerData = mmap(nullptr, bufferSize, PROT_READ | PROT_WRITE,
	                       MAP_SHARED, bufferFd.get(), 0);
	if (bayerData == MAP_FAILED) {
		LOG(SoftIsp, Error) << "Failed to mmap buffer";
		frameDone.emit(frame, bufferId);
		return;
	}

	// TODO: Read AWB/AE parameters from 'results' ControlList
	// These were computed in processStats via metadataReady
	float redGain = 1.0f;
	float blueGain = 1.0f;
	
	const auto *awbGain = results.get(controls::AwbGain);
	if (awbGain && awbGain->size() >= 2) {
		redGain = (*awbGain)[0];
		blueGain = (*awbGain)[1];
	}

	// Convert SBGGR10 Bayer to float array for ONNX input
	// This is a simplified conversion - real implementation needs proper unpacking
	std::vector<float> bayerFloat;
	bayerFloat.reserve(width * height);
	
	// TODO: Properly unpack SBGGR10 packed format
	// For now, just read as bytes and normalize
	uint8_t *byteData = static_cast<uint8_t*>(bayerData);
	for (size_t i = 0; i < width * height; i++) {
		// Normalize to [0, 1]
		float pixel = static_cast<float>(byteData[i % bufferSize]) / 255.0f;
		bayerFloat.push_back(pixel);
	}

	// Apply AWB gains to Bayer data (pre-processing)
	for (size_t i = 0; i < bayerFloat.size(); i++) {
		// Simple demosaic + AWB (replace with proper ISP pipeline)
		size_t row = i / width;
		size_t col = i % width;
		
		// SBGGR pattern: even-even=G, odd-odd=G, even-odd=B, odd-even=R
		bool evenRow = (row % 2 == 0);
		bool evenCol = (col % 2 == 0);
		
		if (evenRow && evenCol) {
			// G pixel
			// bayerFloat[i] *= 1.0f;
		} else if (!evenRow && !evenCol) {
			// G pixel
			// bayerFloat[i] *= 1.0f;
		} else if (evenRow && !evenCol) {
			// B pixel - apply blue gain
			bayerFloat[i] *= blueGain;
		} else {
			// R pixel - apply red gain
			bayerFloat[i] *= redGain;
		}
	}

	// Run applier.onnx inference (Bayer → RGB/YUV)
	std::vector<float> rgbOutput;
	int ret = impl_->applierEngine.runInference(bayerFloat, rgbOutput);
	if (ret < 0) {
		LOG(SoftIsp, Error) << "applier.onnx inference failed";
		munmap(bayerData, bufferSize);
		frameDone.emit(frame, bufferId);
		return;
	}

	LOG(SoftIsp, Debug) << "applier.onnx output size: " << rgbOutput.size();

	// Write RGB output back to buffer (or to separate output buffer)
	// TODO: Convert RGB to YUV if needed, write to correct format
	// For now, just write RGB floats back (simplified)
	
	// In real implementation:
	// 1. Convert float RGB to appropriate format (RGB888, NV12, etc.)
	// 2. Write to bufferFd (or separate output buffer FD)
	// 3. Handle plane offsets for multi-plane formats
	
	// Example: Write as RGB floats (placeholder)
	size_t outputSize = std::min(rgbOutput.size(), width * height * 3);
	for (size_t i = 0; i < outputSize; i++) {
		size_t byteIdx = i % bufferSize;
		byteData[byteIdx] = static_cast<uint8_t>(rgbOutput[i] * 255.0f);
	}

	// Unmap buffer
	munmap(bayerData, bufferSize);

	// Signal completion (Stage 2 done)
	frameDone.emit(frame, bufferId);

	LOG(SoftIsp, Debug) << "processFrame complete for frame " << frame;
}
