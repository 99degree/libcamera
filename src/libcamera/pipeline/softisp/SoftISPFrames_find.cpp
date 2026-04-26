/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

SoftISPFrameInfo *SoftISPFrames::find(unsigned int frame)
{
	auto it = frameInfo_.find(frame);
	return (it != frameInfo_.end()) ? it->second.get() : nullptr;
}
