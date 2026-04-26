/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

SoftIsp::SoftIsp()
{
	LOG(SoftIsp, Info) << "[IPA] SoftIsp::SoftIsp() - begin";
	LOG(SoftIsp, Info) << "[IPA] Creating Impl (which constructs OnnxEngine x2)...";
	impl_ = std::make_unique<Impl>();
	LOG(SoftIsp, Info) << "[IPA] Impl created";
	LOG(SoftIsp, Info) << "[IPA] SoftIsp::SoftIsp() - end (constructor)";
}