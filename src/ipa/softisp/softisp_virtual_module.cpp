/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * SoftISP IPA Module for "dummysoftisp" pipeline
 */

#include "softisp.h"

#include <libcamera/base/log.h>
#include <libcamera/ipa/ipa_module_info.h>

namespace libcamera {
namespace ipa::soft {

// SoftIsp class implementation is in softisp.cpp

} /* namespace ipa::soft */

/* External IPA module interface */
extern "C" {

const struct IPAModuleInfo ipaModuleInfo = {
	IPA_MODULE_API_VERSION,
	0,
	"dummysoftisp",
	"SoftISP Virtual",
};

IPAInterface *ipaCreate() {
	return new ipa::soft::SoftIsp();
}

} /* extern "C" */

} /* namespace libcamera */
