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

	/* Compute fresh stats */
	ControlList fresh;

	// Mock values: simulate stats from algo.onnx
	LOG(SoftIsp, Info) << "[IPA-pS] Generating mock statistics";

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

	LOG(SoftIsp, Info) << "[IPA-pS] Mock outputs:";
	LOG(SoftIsp, Info) << "  ColourGains: " << redGain << ", " << blueGain;
	LOG(SoftIsp, Info) << "  ExposureTime: " << exposureTime;
	LOG(SoftIsp, Info) << "  AnalogueGain: " << analogueGain;

	/* Return computed stats to caller */
	metadataReady.emit(frame, fresh);

	auto us = std::chrono::duration_cast<std::chrono::microseconds>(
		std::chrono::high_resolution_clock::now() - _ps_start).count();
	LOG(SoftIsp, Info) << "[IPA-pS] EXIT - frame=" << frame << " buf=" << bufferId
			   << " dur=" << us << "us";
}