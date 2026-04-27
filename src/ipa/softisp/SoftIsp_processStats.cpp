/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <chrono>
#include <libcamera/base/log.h>

void SoftIsp::processStats(uint32_t frame, [[maybe_unused]] uint32_t bufferId, [[maybe_unused]] const ControlList &stats)
{
	auto _ps_start = std::chrono::high_resolution_clock::now();

	const char *mode;
	if (frame % 4 != 0) {
		// Use cached stats
		ControlList metadata = impl_->cachedStats;
		metadataReady.emit(frame, metadata);
		mode = "SKIP";
	} else {
		// Compute fresh stats via algo.onnx inference
		ControlList fresh;

		if (impl_->algoEngine.isLoaded()) {
			auto _inf_start = std::chrono::high_resolution_clock::now();

			// Build input vector from current frame stats
			std::vector<float> inputs = {
				// Placeholder: real stats from stats parameter
				0.0f, 0.0f, 0.0f, 0.0f
			};
			std::vector<float> outputs;

			int ret = impl_->algoEngine.runInference(inputs, outputs);

			auto _inf_us = std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::high_resolution_clock::now() - _inf_start).count();
			LOG(SoftIsp, Info) << "[IPA-pS] inference dur=" << _inf_us << "us"
					  << " ret=" << ret << " out_sz=" << outputs.size();

			if (ret == 0 && !outputs.empty()) {
				// Map ONNX outputs to ControlList by output name
				const auto &outNames = impl_->algoEngine.getOutputNames();

				for (size_t i = 0; i < outputs.size() && i < outNames.size(); i++) {
					std::string name = outNames[i];
					if (name == "red_gain")
						fresh.set(controls::ColourGains,
							  Span<const float, 2>({outputs[i], 0.0f}));
					else if (name == "blue_gain")
						fresh.set(controls::ColourGains,
							  Span<const float, 2>({0.0f, outputs[i]}));
					else if (name == "exposure_time")
						fresh.set(controls::ExposureTime,
							  static_cast<int32_t>(outputs[i]));
					else if (name == "analogue_gain")
						fresh.set(controls::AnalogueGain, outputs[i]);
					// TODO: add more output mappings
				}
			}
		}

		impl_->cachedStats.merge(fresh);
		metadataReady.emit(frame, impl_->cachedStats);
		mode = "COMPUTE";
	}

	auto us = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::high_resolution_clock::now() - _ps_start).count();
	LOG(SoftIsp, Info) << "[IPA-pS] frame=" << frame << " buf=" << bufferId
			   << " " << mode << " dur=" << us << "us";
}