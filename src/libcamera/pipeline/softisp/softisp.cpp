/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024 George Chan <gchan9527@gmail.com>
 *
 * Pipeline handler for SoftISP (Skeleton)
 */

#include "softisp.h"

#include "softisp_camera.h"
#include "virtual_camera.h"

#include <libcamera/base/log.h>
#include <libcamera/camera_manager.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPPipeline)

// Static member definitions
bool PipelineHandlerSoftISP::created_ = false;
bool PipelineHandlerSoftISP::s_virtualCameraRegistered = false;

// ============================================================================
// Include all method implementations (one file per method)
// ============================================================================
#include "softisp_constructor.cpp"
#include "softisp_destructor.cpp"
#include "softisp_match.cpp"
#include "softisp_generateConfiguration.cpp"
#include "softisp_configure.cpp"
#include "softisp_exportFrameBuffers.cpp"
#include "softisp_start.cpp"
#include "softisp_stopDevice.cpp"
#include "softisp_queueRequestDevice.cpp"
#include "softisp_cameraData.cpp"

// ============================================================================
// REGISTER_PIPELINE_HANDLER macro
// ============================================================================
REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "SoftISP")

} // namespace libcamera
