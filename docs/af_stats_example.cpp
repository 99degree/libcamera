/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * \file af_stats_example.cpp
 * \brief Example: How to calculate AF statistics in SoftISP
 *
 * This example shows how to integrate AF statistics calculation
 * into your SoftISP implementation.
 */

#include "libcamera/internal/software_isp/swisp_stats.h"
#include "libipa/af_stats.h"

#include <libcamera/base/log.h>

namespace libcamera {

/**
 * \brief Example function to calculate AF stats from raw frame data
 *
 * Call this function in your SoftISP after processing the frame,
 * before writing the statistics.
 *
 * \param rawFrame Pointer to raw Bayer frame data
 * \param width Frame width in pixels
 * \param height Frame height in pixels
 * \param stride Stride in bytes
 * \param bitsPerPixel Bits per pixel (8 or 16)
 * \param stats Pointer to SwIspStats structure to fill
 * \param afCalculator Optional: Pre-configured AfStatsCalculator
 */
void calculateAfStats(const uint8_t *rawFrame, uint32_t width, uint32_t height,
                      uint32_t stride, uint32_t bitsPerPixel,
                      SwIspStats *stats,
                      libipa::AfStatsCalculator *afCalculator = nullptr)
{
	if (!stats || !rawFrame)
		return;

	// Initialize AF stats to zero
	stats->afContrast = 0.0f;
	stats->afRawContrast = 0.0f;
	stats->afValidPixels = 0;
	stats->afPhaseError = 0.0f;
	stats->afPhaseConfidence = 0.0f;

	// Create calculator if not provided
	std::unique_ptr<libipa::AfStatsCalculator> localCalculator;
	if (!afCalculator) {
		localCalculator = std::make_unique<libipa::AfStatsCalculator>();
		afCalculator = localCalculator.get();

		// Configure calculator (optional)
		afCalculator->setMethod(libipa::AfStatsCalculator::Method::Sobel);

		// Set up weighted ROIs (center-weighted)
		Rectangle mainRoi(width / 4, height / 4, width / 2, height / 2);
		afCalculator->addWeightedRoi(mainRoi, 1.0f);

		// Add corner ROIs
		afCalculator->addWeightedRoi(Rectangle(0, 0, width / 4, height / 4), 0.3f);
		afCalculator->addWeightedRoi(Rectangle(width * 3 / 4, 0, width / 4, height / 4), 0.3f);
		afCalculator->addWeightedRoi(Rectangle(0, height * 3 / 4, width / 4, height / 4), 0.3f);
		afCalculator->addWeightedRoi(Rectangle(width * 3 / 4, height * 3 / 4, width / 4, height / 4), 0.3f);
	}

	// Calculate AF statistics
	libipa::AfStats afStats;
	if (bitsPerPixel == 16) {
		afStats = afCalculator->calculate16(
			reinterpret_cast<const uint16_t *>(rawFrame),
			width, height, stride / 2);
	} else {
		afStats = afCalculator->calculate(rawFrame, width, height, stride, 8);
	}

	// Store results in stats structure
	stats->afContrast = afStats.contrast;
	stats->afRawContrast = afStats.rawContrast;
	stats->afValidPixels = afStats.validPixels;
	stats->afPhaseError = afStats.phaseError;
	stats->afPhaseConfidence = afStats.phaseConfidence;

	LOG(LibipaAfStats, Debug) << "AF stats: contrast=" << afStats.contrast
	                          << " raw=" << afStats.rawContrast
	                          << " pixels=" << afStats.validPixels;
}

} // namespace libcamera

/**
 * Example usage in your SoftISP implementation:
 *
 * void SoftIspImpl::processFrame(uint32_t frame, uint32_t bufferId,
 *                                const uint8_t *rawFrame,
 *                                uint32_t width, uint32_t height,
 *                                uint32_t stride)
 * {
 *     // ... your existing ISP processing ...
 *
 *     // Calculate statistics
 *     SwIspStats stats = {};
 *     stats.valid = true;
 *
 *     // Calculate color and histogram stats (existing code)
 *     calculateColorStats(rawFrame, width, height, stride, &stats);
 *
 *     // Calculate AF stats (NEW)
 *     calculateAfStats(rawFrame, width, height, stride, 8, &stats);
 *
 *     // Write stats to shared memory
 *     writeStatsToSharedMemory(stats);
 * }
 *
 * For PDAF support, you would also calculate phase error:
 *
 * void calculatePdafStats(const uint8_t *rawFrame, uint32_t width, uint32_t height,
 *                         SwIspStats *stats)
 * {
 *     // Extract PDAF pixels from the frame
 *     // Calculate phase difference between left/right or top/bottom views
 *     // Set stats->afPhaseError and stats->afPhaseConfidence
 * }
 */
