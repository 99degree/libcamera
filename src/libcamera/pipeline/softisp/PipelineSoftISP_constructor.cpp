/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

bool PipelineHandlerSoftISP::created_ = false;
bool PipelineHandlerSoftISP::s_virtualCameraRegistered = false;

PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager)
	: PipelineHandler(manager)
{
}

PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
}

} /* namespace libcamera */
