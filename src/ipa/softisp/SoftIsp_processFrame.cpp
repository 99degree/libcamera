/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <chrono>
#include <libcamera/base/log.h>

void SoftIsp::processFrame(const uint32_t frame,
			   uint32_t bufferId,
			   [[maybe_unused]] const SharedFD &bufferFd,
			   [[maybe_unused]] const int32_t planeIndex,
			   [[maybe_unused]] const int32_t width,
			   [[maybe_unused]] const int32_t height,
			   [[maybe_unused]] const ControlList &results)
{
	LOG(SoftIsp, Debug) << "[IPA-pF] ENTRY - frame=" << frame << " buf=" << bufferId
			  << " this=" << (void*)this << " impl_=" << (void*)impl_.get();

	auto _pf_start = std::chrono::high_resolution_clock::now();

	if (!impl_) {
		LOG(SoftIsp, Error) << "[IPA-pF] ERROR - impl_ is NULL!";
		return;
	}

	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "[IPA-pF] WARNING - processFrame called before init";
		return;
	}

	LOG(SoftIsp, Info) << "[IPA-pF] Processing frame with mock applier.onnx output";

	// Check ControlList values
	LOG(SoftIsp, Info) << "[IPA-pF] Inputs from ControlList:";
	auto colourGains = results.get(controls::ColourGains);
	if (colourGains) {
		LOG(SoftIsp, Info) << "  ColourGains: " << (*colourGains)[0] << ", " << (*colourGains)[1];
	} else {
		LOG(SoftIsp, Info) << "  ColourGains: NOT provided";
	}

	auto exposureTime = results.get(controls::ExposureTime);
	if (exposureTime) {
		LOG(SoftIsp, Info) << "  ExposureTime: " << *exposureTime;
	} else {
		LOG(SoftIsp, Info) << "  ExposureTime: NOT provided";
	}

	auto analogueGain = results.get(controls::AnalogueGain);
	if (analogueGain) {
		LOG(SoftIsp, Info) << "  AnalogueGain: " << *analogueGain;
	} else {
		LOG(SoftIsp, Info) << "  AnalogueGain: NOT provided";
	}

	// Mock processing duration
	usleep(10000); // Simulate some processing time (10ms)

        // Debug: list all ControlList items
        LOG(SoftIsp, Info) << "[IPA-pF] DEBUG ControlList contains " << results.size() << " items:";
        for (const auto &[id, value] : results) {
            LOG(SoftIsp, Info) << "  - id=" << id << " type=" << value.type();
        }

	LOG(SoftIsp, Info) << "[IPA-pF] emitting frameDone";
	frameDone.emit(frame, bufferId);

	auto _pf_end = std::chrono::high_resolution_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(_pf_end - _pf_start).count();
	LOG(SoftIsp, Info) << "[IPA-pF] EXIT - frame=" << frame << " buf=" << bufferId << " dur=" << us << "us";
}