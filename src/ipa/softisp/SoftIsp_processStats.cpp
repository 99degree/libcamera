/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <chrono>
#include <libcamera/base/log.h>

void SoftIsp::processStats(uint32_t frame, [[maybe_unused]] uint32_t bufferId, [[maybe_unused]] const ControlList &stats)
{
	LOG(SoftIsp, Debug) << "[IPA-pS] ENTRY - frame=" << frame << " buf=" << bufferId
			  << " this=" << (void*)this << " impl_=" << (void*)impl_.get();

	auto _ps_start = std::chrono::high_resolution_clock::now();

	if (!impl_) {
		LOG(SoftIsp, Error) << "[IPA-pS] ERROR - impl_ is NULL!";
		return;
	}

	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "[IPA-pS] WARNING - processStats called before init";
		return;
	}

	/* Compute fresh stats via algo.onnx inference */
	ControlList fresh;

	if (impl_->algoEngine.isLoaded()) {
		LOG(SoftIsp, Info) << "[IPA-pS] algoEngine loaded, running inference";
		auto _inf_start = std::chrono::high_resolution_clock::now();

		// Simulate input from statistics
		std::vector<float> inputs = {
			0.5f, 0.5f, 0.5f, 0.5f  // avg_r, avg_b, avg_g, avg_y
		};
		std::vector<float> outputs;

		int ret = impl_->algoEngine.runInference(inputs, outputs);

		auto _inf_us = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::high_resolution_clock::now() - _inf_start).count();
		LOG(SoftIsp, Info) << "[IPA-pS] inference dur=" << _inf_us << "us"
				  << " ret=" << ret << " out_sz=" << outputs.size();

		if (ret == 0 && !outputs.empty()) {
			LOG(SoftIsp, Info) << "[IPA-pS] mapping outputs to ControlList";
			const auto &outNames = impl_->algoEngine.getOutputNames();

			for (size_t i = 0; i < outputs.size() && i < outNames.size(); i++) {
				std::string name = outNames[i];
				float value = outputs[i];
				LOG(SoftIsp, Debug) << "[IPA-pS] output[" << i << "] name=" << name << " val=" << value;
				
				// Map ONNX outputs to standard controls
				if (name == "red_gain" || name == "awb_red") {
					fresh.set(controls::ColourGains, Span<const float, 2>({value, 1.0f}));
				}
				else if (name == "blue_gain" || name == "awb_blue") {
					fresh.set(controls::ColourGains, Span<const float, 2>({1.0f, value}));
				}
				else if (name == "exposure_time" || name == "ae_exposure") {
					fresh.set(controls::ExposureTime, static_cast<int32_t>(value));
				}
				else if (name == "analogue_gain" || name == "ae_gain") {
					fresh.set(controls::AnalogueGain, value);
				}
			}
		}
	} else {
		LOG(SoftIsp, Info) << "[IPA-pS] algoEngine NOT loaded, using mock values";

		// Mock values: simulate stats from algo.onnx
		// AWB (Auto White Balance) gains
		float redGain = 1.5f;
		float blueGain = 1.2f;
		fresh.set(controls::ColourGains, Span<const float, 2>({redGain, blueGain}));

		// Exposure
		int exposureTime = 2000; // 2ms
		fresh.set(controls::ExposureTime, exposureTime);

		// Gain
		float analogueGain = 2.5f;
		fresh.set(controls::AnalogueGain, analogueGain);
	}

	LOG(SoftIsp, Info) << "[IPA-pS] Generated stats:";
	auto colourGains = fresh.get(controls::ColourGains);
	if (colourGains) {
		LOG(SoftIsp, Info) << "  ColourGains: " << (*colourGains)[0] << ", " << (*colourGains)[1];
	}
	auto exposureTime = fresh.get(controls::ExposureTime);
	if (exposureTime) {
		LOG(SoftIsp, Info) << "  ExposureTime: " << *exposureTime;
	}
	auto analogueGain = fresh.get(controls::AnalogueGain);
	if (analogueGain) {
		LOG(SoftIsp, Info) << "  AnalogueGain: " << *analogueGain;
	}

	/* Return computed stats to caller */
	metadataReady.emit(frame, fresh);

	auto us = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::high_resolution_clock::now() - _ps_start).count();
	LOG(SoftIsp, Info) << "[IPA-pS] EXIT - frame=" << frame << " buf=" << bufferId
			   << " dur=" << us << "us";
}