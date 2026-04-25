/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId,
			   libcamera::ControlList &stats)
{
	// TODO: Implement processStats
	// Extract statistics from stats buffer and compute AWB/AE parameters
	// For now, just emit default values
	(void)frame;
	(void)bufferId;
	
	// Add default AWB gains
	stats.set(controls::AeState, controls::AeStateConverged);
}
