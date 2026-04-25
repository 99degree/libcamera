/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstring>

void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId,
			   const libcamera::ControlList &stats)
{
	if (!impl_->initialized)
		return;

	LOG(IPASoftISP, Debug) << "processStats: frame=" << frame << " bufferId=" << bufferId;

	// TODO: Extract statistics from stats buffer
	// For now, we'll use placeholder values
	std::vector<float> statsInput = {1.0f, 1.0f, 1.0f, 1.0f}; // R gain, B gain, exposure, etc.

	// Run algo.onnx inference to get AWB/AE parameters
	std::vector<float> algoOutput;
	int ret = impl_->algoEngine.runInference(statsInput, algoOutput);
	if (ret < 0) {
		LOG(IPASoftISP, Error) << "algoEngine inference failed: " << ret;
		return;
	}

	// Extract AWB gains from output (assuming first 2 values are R and B gains)
	if (algoOutput.size() >= 2) {
		float redGain = algoOutput[0];
		float blueGain = algoOutput[1];
		LOG(IPASoftISP, Debug) << "AWB gains: R=" << redGain << " B=" << blueGain;
	}

	// TODO: Populate stats ControlList with AWB/AE results
	// For now, just emit a basic stats update
	(void)stats;
}
