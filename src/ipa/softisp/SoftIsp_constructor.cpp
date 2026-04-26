/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

SoftIsp::SoftIsp()
{
	impl_ = std::make_unique<Impl>();
	LOG(SoftIsp, Info) << "SoftIsp constructor";
}