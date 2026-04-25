/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <chrono>
#include <libcamera/base/log.h>

void SoftIsp::processStats(uint32_t frame, uint32_t bufferId, const libcamera::ControlList &stats)
{
    auto _ps_start = std::chrono::high_resolution_clock::now();
    LOG(IPASoftISP, Info) << "processStats: frame=" << frame << ", bufferId=" << bufferId;
    auto _ps_end = std::chrono::high_resolution_clock::now();
    auto _ps_us = std::chrono::duration_cast<std::chrono::microseconds>(_ps_end - _ps_start).count();
    LOG(IPASoftISP, Info) << "processStats completed in " << _ps_us << " μs";
}
