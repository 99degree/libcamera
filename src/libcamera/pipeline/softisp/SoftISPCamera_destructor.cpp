/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

SoftISPCameraData::~SoftISPCameraData()
{
	Thread::exit(false);
	wait();
}

} /* namespace libcamera */
