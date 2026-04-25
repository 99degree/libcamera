/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int32_t SoftIsp::start()
{
	if (!impl_->initialized) {
		return -ENODEV;
	}

	return 0;
}
