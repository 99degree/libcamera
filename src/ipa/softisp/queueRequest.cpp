/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftIsp::queueRequest(const uint32_t frame, const ControlList & /*sensorControls*/)
{
	LOG(SoftIsp, Debug) << "queueRequest: frame=" << frame;
}
