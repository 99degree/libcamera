/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

SoftISPCameraData::~SoftISPCameraData()
{
	LOG(SoftISPPipeline, Info) << "Destroying SoftISPCameraData";
	if (running_ && frameGenerator_) {
		frameGenerator_->stop();
	}
}
