/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftISPCameraData::stop()
{
	LOG(SoftISPPipeline, Info) << "Stopping camera";
	if (frameGenerator_) {
		frameGenerator_->stop();
	}
	running_ = false;
}
