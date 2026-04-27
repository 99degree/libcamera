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
	auto _pf_start = std::chrono::high_resolution_clock::now();

	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "processFrame called before init";
		return;
	}

	frameDone.emit(frame, bufferId);

	auto _pf_end = std::chrono::high_resolution_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(_pf_end - _pf_start).count();
	LOG(SoftIsp, Info) << "[IPA-pF] frame=" << frame << " buf=" << bufferId << " dur=" << us << "us";
}