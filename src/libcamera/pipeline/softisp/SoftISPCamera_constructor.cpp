/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
	: Camera::Private(pipe)
{
	LOG(SoftISPPipeline, Info) << "SoftISPCameraData created";
}
