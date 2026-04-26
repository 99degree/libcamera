/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
	LOG(SoftISPPipeline, Info) << "Destroying PipelineHandlerSoftISP";
	// NOTE: Do NOT reset created_ here - it must persist across
	// multiple pipeline handler instances during probing.
}
