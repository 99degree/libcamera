/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftIsp::computeParams(const uint32_t frame)
{
	if (!impl_->initialized) {
		return;
	}

	// TODO: Use algoEngine to compute parameters if needed
}
