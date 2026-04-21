/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * IPA Module for SoftISP (real cameras)
 */

#include "softisp.h"

#include <libcamera/base/log.h>
#include <libcamera/ipa/ipa_module_info.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPModule)

namespace ipa::soft {

} /* namespace ipa::soft */

/*
 * ipaCreate() - Create a new SoftISP IPA instance.
 */
extern "C" IPAInterface *ipaCreate()
{
	LOG(SoftISPModule, Info) << "Creating SoftISP IPA module (real camera)";
	return new ipa::soft::SoftIsp();
}

} /* namespace libcamera */
