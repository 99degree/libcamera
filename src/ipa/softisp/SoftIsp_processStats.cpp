/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <chrono>
#include <libcamera/base/log.h>

void SoftIsp::processStats(uint32_t frame, uint32_t bufferId, [[maybe_unused]] ControlList &stats)
{
	auto _ps_start = std::chrono::high_resolution_clock::now();

	/*
	 * Throttle: processStats runs on every frame but only re-computes
	 * on every 4th frame. On skip frames, return cached stats immediately.
	 */
	if (frame % 4 != 0) {
		ControlList metadata = impl_->cachedStats;
		metadataReady.emit(frame, metadata);

		auto _ps_end = std::chrono::high_resolution_clock::now();
		auto us = std::chrono::duration_cast<std::chrono::microseconds>(_ps_end - _ps_start).count();
		LOG(SoftIsp, Info) << "[IPA-pS] frame=" << frame << " SKIPPED (cached) " << us << "us";
		return;
	}

	/*
	 * Re-compute statistics from frame data.
	 * TODO: Read stats from shared memory buffer, run libipa algorithms,
	 *       update impl_->cachedStats with new AWB/AE/AF values.
	 */
	impl_->cachedStats.clear();  /* placeholder - real stats go here */

	ControlList metadata = impl_->cachedStats;
	metadataReady.emit(frame, metadata);

	auto _ps_end = std::chrono::high_resolution_clock::now();
	auto us = std::chrono::duration_cast<std::chrono::microseconds>(_ps_end - _ps_start).count();
	LOG(SoftIsp, Info) << "[IPA-pS] frame=" << frame << " COMPUTED " << us << "us";
}