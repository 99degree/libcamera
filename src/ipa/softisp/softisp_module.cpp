/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * SoftISP IPA Module
 */

#include "softisp.h"
#include <libcamera/base/log.h>
#include <libcamera/ipa/ipa_module_info.h>

namespace libcamera {
namespace ipa {
namespace soft {
// SoftIsp class implementation is in softisp.cpp
} /* namespace soft */
} /* namespace ipa */

/* External IPA module interface */
extern "C" {
	const struct IPAModuleInfo ipaModuleInfo = {
		IPA_MODULE_API_VERSION,
		0,
		"softisp",
		"SoftISP",
	};

	IPAInterface *ipaCreate()
	{
		return new ipa::soft::SoftIsp();
	}
} /* extern "C" */
} /* namespace libcamera */
