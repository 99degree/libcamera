/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <chrono>
#include <libcamera/base/log.h>

void SoftIsp::processFrame(const uint32_t frame,
			   uint32_t bufferId,
			   const SharedFD &bufferFd,
			   const int32_t planeIndex,
			   const int32_t width,
			   const int32_t height,
			   [[maybe_unused]] const ControlList &results)
{
	auto _pf_start = std::chrono::high_resolution_clock::now();

	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "processFrame called before init";
		return;
	}

	ensureModelsLoaded();

	ControlList metadata;
	metadataReady.emit(frame, metadata);
	frameDone.emit(frame, bufferId);

	auto _pf_end = std::chrono::high_resolution_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(_pf_end - _pf_start).count();
	LOG(SoftIsp, Info) << "[IPA-pF] frame=" << frame << " took " << us << "us";
}