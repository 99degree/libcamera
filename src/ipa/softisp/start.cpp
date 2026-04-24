/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int32_t SoftIsp::start()
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Error) << "Not initialized";
		return -ENODEV;
	}

	LOG(SoftIsp, Info) << "SoftISP started";
	return 0;
}
