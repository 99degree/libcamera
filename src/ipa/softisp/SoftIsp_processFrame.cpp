/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>
#include <cmath>

void SoftIsp::processFrame(const uint32_t frame, const uint32_t bufferId,
			   const libcamera::SharedFD &bufferFd,
			   const int32_t /*planeIndex*/,
			   const int32_t width, const int32_t height,
			   const libcamera::ControlList &results)
{
	(void)results; // Unused for now
	
	if (!impl_->initialized) {
		frameDone.emit(frame, bufferId);
		return;
	}

	LOG(IPASoftISP, Debug) << "processFrame: frame=" << frame 
			      << " size=" << width << "x" << height;

	// Calculate buffer size for SBGGR10 (10 bits per pixel, packed)
	// Each row is padded to 4-byte boundary
	size_t bytesPerRow = ((width * 10 + 31) / 32) * 4;
	size_t bufferSize = bytesPerRow * height;

	// Map the buffer for reading/writing
	void *bayerData = mmap(nullptr, bufferSize, PROT_READ | PROT_WRITE, 
			       MAP_SHARED, bufferFd.get(), 0);
	if (bayerData == MAP_FAILED) {
		LOG(IPASoftISP, Error) << "Failed to map buffer: " << strerror(errno);
		frameDone.emit(frame, bufferId);
		return;
	}

	// Convert Bayer data to float array for ONNX input
	// SBGGR10 format: 10-bit pixels packed, G R G R ... / B G B G ...
	std::vector<float> bayerFloat;
	bayerFloat.reserve(width * height);

	uint8_t *byteData = static_cast<uint8_t*>(bayerData);
	for (int y = 0; y < height; y++) {
		uint8_t *rowPtr = byteData + y * bytesPerRow;
		for (int x = 0; x < width; x++) {
			// Extract 10-bit pixel value (simplified - actual extraction depends on packing)
			size_t bitOffset = (y * width + x) * 10;
			size_t byteOffset = bitOffset / 8;
			size_t bitInByte = bitOffset % 8;
			
			if (byteOffset + 1 < bufferSize) {
				uint16_t pixel = (rowPtr[byteOffset] | (rowPtr[byteOffset + 1] << 8)) 
						 >> bitInByte;
				pixel &= 0x3FF; // Mask to 10 bits
				
				// Apply AWB gains
				float pixelValue = static_cast<float>(pixel) / 1023.0f;
				
				// Simple Bayer pattern: G R / B G
				bool isRowEven = (y % 2 == 0);
				bool isColEven = (x % 2 == 0);
				
				if (isRowEven && isColEven) {
					// G pixel
					pixelValue = pixelValue;
				} else if (isRowEven && !isColEven) {
					// R pixel - apply red gain
					pixelValue = pixelValue * impl_->currentRedGain;
				} else if (!isRowEven && isColEven) {
					// B pixel - apply blue gain
					pixelValue = pixelValue * impl_->currentBlueGain;
				} else {
					// G pixel
					pixelValue = pixelValue;
				}
				
				// Clamp to [0, 1]
				pixelValue = std::max(0.0f, std::min(1.0f, pixelValue));
				bayerFloat.push_back(pixelValue);
			} else {
				bayerFloat.push_back(0.0f);
			}
		}
	}

	// Run applier.onnx inference (Bayer → processed output)
	std::vector<float> outputFloat;
	int ret = impl_->applierEngine.runInference(bayerFloat, outputFloat);
	if (ret < 0) {
		LOG(IPASoftISP, Error) << "applierEngine inference failed: " << ret;
		munmap(bayerData, bufferSize);
		frameDone.emit(frame, bufferId);
		return;
	}

	LOG(IPASoftISP, Debug) << "applier.onnx output size: " << outputFloat.size();

	// Write output back to buffer
	// For now, just write the processed values back as 8-bit (simplified)
	size_t outputSize = std::min(outputFloat.size(), 
				     static_cast<size_t>(width * height));
	
	for (size_t i = 0; i < outputSize; i++) {
		// Convert float [0,1] to 8-bit [0,255]
		uint8_t value = static_cast<uint8_t>(
			std::min(255.0f, std::max(0.0f, outputFloat[i] * 255.0f)));
		
		// Write back to buffer (simplified - actual format depends on output)
		if (i < bufferSize) {
			byteData[i] = value;
		}
	}

	// Unmap buffer
	munmap(bayerData, bufferSize);

	// Emit frame done signal
	frameDone.emit(frame, bufferId);
	LOG(IPASoftISP, Debug) << "Frame processed successfully";
}
