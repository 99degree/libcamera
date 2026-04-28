/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <chrono>
#include <libcamera/base/log.h>

void SoftIsp::processStats(uint32_t frame, [[maybe_unused]] uint32_t bufferId, [[maybe_unused]] const ControlList &stats)
{
	LOG(SoftIsp, Debug) << "[IPA-pS] ENTRY - frame=" << frame << " buf=" << bufferId << " this=" << (void*)this << " impl_=" << (void*)impl_.get();
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

		// Use model's requirements
		std::vector<float> inferenceInput;
		int width = impl_->imageWidth > 0 ? impl_->imageWidth : 640;
		int height = impl_->imageHeight > 0 ? impl_->imageHeight : 480;

		// Create inputs based on known model requirements
		for (const auto &name : impl_->algoEngine.getInputNames()) {
			if (name == std::string("image_desc.input.image.function")) {
				// 2D image, fill with chessboard pattern
				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {
						inferenceInput.push_back( ((x+y) % 2) == 0 ? 0.25f : 0.75f );
					}
				}
			} else if (name == std::string("image_desc.input.width.function")) {
				inferenceInput.push_back(static_cast<float>(width));
			} else if (name == std::string("image_desc.input.frame_id.function")) {
				inferenceInput.push_back(static_cast<float>(frame));
			} else if (name == std::string("blacklevel.offset.function")) {
				inferenceInput.push_back(0.05f); // 5% black level
			}
		}

		LOG(SoftIsp, Info) << "[IPA-pS] Created " << inferenceInput.size() << " input values";

		std::vector<float> outputs;
		int ret = impl_->algoEngine.runInference(inferenceInput, outputs);
		auto _inf_us = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::high_resolution_clock::now() - _inf_start).count();
		LOG(SoftIsp, Info) << "[IPA-pS] inference dur=" << _inf_us << "us ret=" << ret << " out_sz=" << outputs.size();

		if (ret == 0 && !outputs.empty()) {
			LOG(SoftIsp, Info) << "[IPA-pS] mapping outputs to ControlList";
			const auto &outNames = impl_->algoEngine.getOutputNames();
			for (size_t i = 0; i < outputs.size() && i < outNames.size(); i++) {
				std::string name = outNames[i];
				float value = outputs[i];
				LOG(SoftIsp, Debug) << "[IPA-pS] output[" << i << "] name=" << name << " val=" << value;

				// Keep existing mapping logic
				if (name == "red_gain" || name == "awb_red") {
					fresh.set(controls::ColourGains, Span<const float, 2>({value, 1.0f}));
				} else if (name == "blue_gain" || name == "awb_blue") {
					fresh.set(controls::ColourGains, Span<const float, 2>({1.0f, value}));
				} else if (name == "exposure_time" || name == "ae_exposure") {
					fresh.set(controls::ExposureTime, static_cast<int32_t>(value));
				} else if (name == "analogue_gain" || name == "ae_gain") {
					fresh.set(controls::AnalogueGain, value);
				}

				// Additional output handling
				if (name == "awb.wb_gains.function") {
					impl_->awbGains.clear();
					const auto &info = impl_->algoEngine.getOutputInfo().at(name);
					for (size_t j = 0; j < info.elementCount && (i + j) < outputs.size(); ++j) {
						impl_->awbGains.push_back(outputs[i + j]);
					}
					i += info.elementCount - 1; // skip tensor values
				}
			}

			// Apply AWB gains from ONNX if available
			if (!impl_->awbGains.empty() && impl_->awbGains.size() >= 3) {
				fresh.set(controls::ColourGains, Span<const float, 2>({impl_->awbGains[0], impl_->awbGains[2]}));
				LOG(SoftIsp, Info) << "[IPA-pS] Applied ONNX AWB: " << impl_->awbGains[0] << ", " << impl_->awbGains[2];
			}
		}
	}

	// Fallback to mock values
	if (fresh.empty()) {
		LOG(SoftIsp, Info) << "[IPA-pS] algoEngine NOT loaded, using mock values";
		fresh.set(controls::ColourGains, Span<const float, 2>({1.5f, 1.2f}));
		fresh.set(controls::ExposureTime, 2000);
		fresh.set(controls::AnalogueGain, 2.5f);
	}

	LOG(SoftIsp, Info) << "[IPA-pS] Generated stats:";
	auto colourGains = fresh.get(controls::ColourGains);
	if (colourGains) {
		LOG(SoftIsp, Info) << " ColourGains: " << (*colourGains)[0] << ", " << (*colourGains)[1];
	}
	auto exposureTime = fresh.get(controls::ExposureTime);
	if (exposureTime) {
		LOG(SoftIsp, Info) << " ExposureTime: " << *exposureTime;
	}
	auto analogueGain = fresh.get(controls::AnalogueGain);
	if (analogueGain) {
		LOG(SoftIsp, Info) << " AnalogueGain: " << *analogueGain;
	}

	metadataReady.emit(frame, fresh);
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::high_resolution_clock::now() - _ps_start).count();
	LOG(SoftIsp, Info) << "[IPA-pS] EXIT - frame=" << frame << " buf=" << bufferId << " dur=" << us << "us";
}
