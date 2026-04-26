/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
	LOG(SoftISPPipeline, Info) << "Destroying PipelineHandlerSoftISP";
	created_ = false;
}
