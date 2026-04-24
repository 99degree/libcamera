/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024 George Chan <gchan9527@gmail.com>
 *
 * SoftISPConfiguration implementation
 */

#include "softisp.h"

namespace libcamera {

SoftISPConfiguration::SoftISPConfiguration()
{
}

CameraConfiguration::Status SoftISPConfiguration::validate()
{
    if (empty())
        return Invalid;

    Status status = Valid;
    for (auto it = begin(); it != end(); ++it) {
        StreamConfiguration &cfg = *it;
        if (cfg.size.width == 0 || cfg.size.height == 0)
            return Invalid;
        if (cfg.pixelFormat == 0)
            return Invalid;
    }
    return status;
}

} // namespace libcamera
