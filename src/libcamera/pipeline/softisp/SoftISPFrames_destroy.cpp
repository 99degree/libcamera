/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPFrames::destroy(unsigned int frame)
{
	auto it = frameInfo_.find(frame);
	if (it == frameInfo_.end()) {
		return -ENOENT;
	}
	
	frameInfo_.erase(it);
	return 0;
}
