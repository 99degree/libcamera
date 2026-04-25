/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <chrono>
#include <libcamera/base/log.h>

void SoftIsp::computeParams(const uint32_t frame)
{
    auto _cp_start = std::chrono::high_resolution_clock::now();
    LOG(SoftIsp, Info) << "computeParams: frame=" << frame;
    auto _cp_end = std::chrono::high_resolution_clock::now();
    auto _cp_us = std::chrono::duration_cast<std::chrono::microseconds>(_cp_end - _cp_start).count();
    LOG(SoftIsp, Info) << "computeParams completed in " << _cp_us << " μs";
}
