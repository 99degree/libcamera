/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int32_t SoftIsp::configure(const IPAConfigInfo & /*configInfo*/)
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Error) << "Not initialized";
		return -ENODEV;
	}

	LOG(SoftIsp, Info) << "SoftISP configured";
	return 0;
}
