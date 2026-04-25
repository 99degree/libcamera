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
    
    LOG(IPASoftISP, Debug) << "=== processStats() started ===";
    
    // Placeholder: Extract stats and calculate AWB/AE
    // In a real implementation, process the stats data here
    
    // Add dummy metadata
    metadata.add(controls::AeState, controls::AeStateConverged);
    metadata.add(controls::AwbState, controls::AwbStateConverged);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    
    LOG(IPASoftISP, Info) << "processStats() completed in " << duration.count() << " μs";
    
    return 0;
}

} /* namespace libcamera */
