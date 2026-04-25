/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstring>
#include <sys/mman.h>

void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId,
			   const libcamera::ControlList & /*sensorControls*/)
{
	if (!impl_->initialized) {
		return;
	}


	// TODO: Read stats from SharedFD (fdStats from init)
	// For now, create dummy stats data
	// In real implementation:
	// 1. Map the stats buffer from fdStats_ (stored in init())
	// 2. Read histogram, AE stats, AWB stats from buffer
	// 3. Convert to float array for ONNX input
	
	std::vector<float> statsInput(256, 0.0f); // Dummy histogram data
	for (int i = 0; i < 256; i++) {
		statsInput[i] = static_cast<float>(i) / 255.0f;
	}

	// Run algo.onnx inference
	std::vector<float> awbAeOutput;
	int ret = impl_->algoEngine.runInference(statsInput, awbAeOutput);
	if (ret < 0) {
		ControlList metadata(controls::controls);
		metadataReady.emit(frame, metadata);
		return;
	}


	// Extract AWB/AE parameters
	ControlList metadata(controls::controls);
	
	if (awbAeOutput.size() >= 3) {
		float redGain = awbAeOutput[0];
		float greenGain = awbAeOutput[1];
		float blueGain = awbAeOutput[2];
		
		// metadata.set(controls::AwbGain, {redGain, blueGain});
	}
	
	if (awbAeOutput.size() >= 6) {
		float exposure = awbAeOutput[3];
		float analogGain = awbAeOutput[4];
		float digitalGain = awbAeOutput[5];
		
	}

	// Signal completion
	metadataReady.emit(frame, metadata);

}
