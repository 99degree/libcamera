/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstring>

void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId,
			   const libcamera::ControlList &stats)
{
	if (!impl_->initialized) {
		frameDone.emit(frame, bufferId);
		return;
	}

	LOG(IPASoftISP, Debug) << "processStats: frame=" << frame << " bufferId=" << bufferId;

	// Extract statistics from the input stats ControlList
	// For now, we'll use placeholder values for demonstration
	// In a real implementation, you would extract:
	// - Red/Blue channel gains from AWB stats
	// - Exposure values from AE stats
	// - Luminance/histogram data
	
	std::vector<float> statsInput(4, 1.0f); // Placeholder: [R_gain, B_gain, exposure, luminance]
	
	// Try to extract real values from stats if available
	if (stats.get(controls::AeState)) {
		// AE state is available, extract exposure value
		statsInput[2] = 1.0f; // Placeholder exposure
	}
	
	if (stats.get(controls::draft::AwbState)) {
		// AWB state is available, extract gains
		statsInput[0] = 1.0f; // Placeholder R gain
		statsInput[1] = 1.0f; // Placeholder B gain
	}

	// Run algo.onnx inference to compute ISP parameters
	std::vector<float> algoOutput;
	int ret = impl_->algoEngine.runInference(statsInput, algoOutput);
	if (ret < 0) {
		LOG(IPASoftISP, Error) << "algoEngine inference failed: " << ret;
		frameDone.emit(frame, bufferId);
		return;
	}

	LOG(IPASoftISP, Debug) << "algo.onnx output size: " << algoOutput.size();

	// Extract AWB gains from algo output (assuming first 2 values are R and B gains)
	float redGain = 1.0f;
	float blueGain = 1.0f;
	
	if (algoOutput.size() >= 2) {
		redGain = std::max(0.1f, std::min(10.0f, algoOutput[0])); // Clamp to reasonable range
		blueGain = std::max(0.1f, std::min(10.0f, algoOutput[1]));
		LOG(IPASoftISP, Debug) << "Computed AWB gains: R=" << redGain << " B=" << blueGain;
	}

	// Store gains for use in processFrame
	impl_->currentRedGain = redGain;
	impl_->currentBlueGain = blueGain;

	// TODO: Extract more parameters from algoOutput (ISO, exposure time, etc.)
	// and populate the results ControlList
	
	// For now, emit frame done
	frameDone.emit(frame, bufferId);
}
