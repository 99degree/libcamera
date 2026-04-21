/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * IPA Module for SoftISP (dummy/virtual cameras)
 */

#include "softisp.h"

#include <libcamera/base/log.h>
#include <libcamera/ipa/ipa_module_info.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPVirtModule)

namespace ipa::soft {

} /* namespace ipa::soft */

/*
 * ipaCreate() - Create a new SoftISP IPA instance for virtual cameras.
 */
extern "C" IPAInterface *ipaCreate()
{
	LOG(SoftISPVirtModule, Info) << "Creating SoftISP IPA module (dummy camera)";
	return new ipa::soft::SoftIsp();
}

} /* namespace libcamera */
