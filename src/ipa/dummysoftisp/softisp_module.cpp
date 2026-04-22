/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * DummySoftISP IPA Module for "dummysoftisp" pipeline
 */
#include "../softisp/softisp.h"
#include <libcamera/base/log.h>
#include <libcamera/ipa/ipa_module_info.h>

namespace libcamera {

/* External IPA module interface */
extern "C" {

const struct IPAModuleInfo ipaModuleInfo = {
	IPA_MODULE_API_VERSION,
	0,
	"dummysoftisp",  /* Must match pipeline name */
	"DummySoftISP",
};

IPAInterface *ipaCreate()
{
	fprintf(stderr, ">>> dummysoftisp ipaCreate() called, creating SoftIsp\n");
	ipa::soft::SoftIsp *isp = new ipa::soft::SoftIsp();
	fprintf(stderr, ">>> dummysoftisp ipaCreate() returned %p\n", isp);
	return isp;
}

} /* extern "C" */

} /* namespace libcamera */
