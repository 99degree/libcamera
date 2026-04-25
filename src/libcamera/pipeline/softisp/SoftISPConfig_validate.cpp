/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

SoftISPConfiguration::Status SoftISPConfiguration::validate()
{
	// TODO: Validate configuration
	// For now, return Valid as placeholder
	return SoftISPConfiguration::Status::Valid;
}

} /* namespace libcamera */
