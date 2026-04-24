/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024 George Chan <gchan9527@gmail.com>
 *
 * SoftISPCameraData - Camera Object implementation (Skeleton)
 */

#include "softisp_camera.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libcamera/base/log.h>
#include <libcamera/camera.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/stream.h>

#include "softisp.h"
#include "virtual_camera.h"

namespace libcamera {

LOG_DECLARE_CATEGORY(SoftISPPipeline)

// ============================================================================
// Include all method implementations (one file per method)
// ============================================================================
#include "softisp_camera_constructor.cpp"
#include "softisp_camera_destructor.cpp"
#include "softisp_camera_init.cpp"
#include "softisp_camera_loadIPA.cpp"
#include "softisp_camera_generateConfiguration.cpp"
#include "softisp_camera_processRequest.cpp"
#include "softisp_camera_getBufferFromId.cpp"
#include "softisp_camera_storeBuffer.cpp"
#include "softisp_camera_run.cpp"
#include "softisp_camera_configure.cpp"
#include "softisp_camera_exportFrameBuffers.cpp"
#include "softisp_camera_start.cpp"
#include "softisp_camera_stop.cpp"
#include "softisp_camera_queueRequest.cpp"

} // namespace libcamera
