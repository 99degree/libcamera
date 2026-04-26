/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

SoftIsp::SoftIsp()
{
	LOG(SoftIsp, Info) << "[IPA] SoftIsp::SoftIsp() - begin";
	impl_ = std::make_unique<Impl>();
	impl_->algoEngine = std::make_unique<OnnxEngineImpl>();
	impl_->applierEngine = std::make_unique<OnnxEngineImpl>();
	LOG(SoftIsp, Info) << "[IPA] Impl created with 2 engines";
	LOG(SoftIsp, Info) << "[IPA] SoftIsp::SoftIsp() - end";
}