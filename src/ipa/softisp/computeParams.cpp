/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftIsp::computeParams(const uint32_t frame)
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "Not initialized";
		return;
	}

	LOG(SoftIsp, Debug) << "computeParams: frame=" << frame;
	// TODO: Use algoEngine to compute parameters if needed
}
