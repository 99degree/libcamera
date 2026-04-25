/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

#include <chrono>
#include <libcamera/base/log.h>

namespace libcamera {

int SoftIsp::processStats(const uint32_t *stats, size_t statsSize,
                          const uint8_t *raw, size_t rawSize,
                          ControlList &metadata)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    // === ORIGINAL LOGIC STARTS HERE ===
    // Placeholder implementation - no actual stats processing yet
    // In a real implementation, this would extract AWB/AE parameters from stats

    // Add some dummy metadata for testing
    metadata.add(controls::AeState, controls::AeStateConverged);
    metadata.add(controls::AwbState, controls::AwbStateConverged);
    metadata.add(controls::Brightness, 0.0f);
    metadata.add(controls::Contrast, 1.0f);
    // === ORIGINAL LOGIC ENDS HERE ===

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);

    LOG(IPASoftISP, Info) << "processStats() completed in " << duration.count() << " μs";

    return 0;
}

} /* namespace libcamera */
