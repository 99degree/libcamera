/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* Copyright (C) 2024 Pipeline handler for SoftISP */
#include "softisp.h"
#include "virtual_camera.h"
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <queue>
#include <sys/mman.h>
#include <unistd.h>
#include <libcamera/base/log.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/geometry.h>
#include "libcamera/internal/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"

namespace libcamera {

static std::map<uint32_t, int> g_bufferFdMap;
LOG_DEFINE_CATEGORY(SoftISPPipeline)
bool PipelineHandlerSoftISP::created_ = false;

SoftISPConfiguration::SoftISPConfiguration() {}

CameraConfiguration::Status SoftISPConfiguration::validate() {
    if (empty()) return Invalid;
    Status status = Valid;
    for (auto it = begin(); it != end(); ++it) {
        StreamConfiguration &cfg = *it;
        if (cfg.size.width == 0 || cfg.size.height == 0) return Invalid;
        if (cfg.pixelFormat == 0) return Invalid;
    }
    return status;
}

