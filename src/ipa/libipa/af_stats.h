/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * \file af_stats.h
 * \brief CPU-based Auto-Focus Statistics Calculator
 *
 * This class calculates contrast-based focus metrics from raw image data.
 * It provides a hardware-agnostic way to compute AF statistics that can be
 * used by the AfAlgo algorithm.
 */
#pragma once

#include <cstdint>
#include <libcamera/geometry.h>
#include <libcamera/span.h>

namespace libcamera {
namespace libipa {

/**
 * \brief AF Statistics structure
 *
 * Contains the focus metrics calculated from an image frame.
 */
struct AfStats {
	/**
	 * \brief Contrast score (0.0 to 1.0, higher = sharper)
	 *
	 * Normalized contrast metric where higher values indicate
	 * a more in-focus image.
	 */
	float contrast;

	/**
	 * \brief Phase detection error in pixels
	 *
	 * Positive value means lens is too close (needs to move away).
	 * Negative value means lens is too far (needs to move closer).
	 * Zero means no phase data available.
	 */
	float phaseError;

	/**
	 * \brief Confidence in phase detection (0.0 to 1.0)
	 *
	 * Only valid if phaseError is non-zero.
	 */
	float phaseConfidence;

	/**
	 * \brief Raw contrast value (unnormalized)
	 *
	 * Useful for debugging and threshold tuning.
	 */
	float rawContrast;

	/**
	 * \brief Number of valid pixels used in calculation
	 */
	uint32_t validPixels;

	/**
	 * \brief Check if statistics are valid
	 */
	bool isValid() const { return validPixels > 0; }
};

/**
 * \class AfStatsCalculator
 * \brief Calculate AF statistics from raw image data
 *
 * This class provides CPU-based calculation of focus metrics from
 * raw Bayer or debayered image data. It supports multiple methods:
 *
 * - **Sobel filter**: Calculates edge strength using Sobel operators
 * - **Laplacian**: Uses Laplacian filter for edge detection
 * - **Variance**: Computes variance of pixel intensities in regions
 *
 * The calculator can work on:
 * - Full resolution frames (slower)
 * - Downsampled regions (faster)
 * - Multiple ROIs for weighted averaging
 *
 * Usage:
 * \code
 * AfStatsCalculator calc;
 * calc.setMethod(AfStatsCalculator::Method::Sobel);
 * calc.setRoi(Rectangle(100, 100, 500, 500));
 *
 * // For each frame:
 * AfStats stats = calc.calculate(frameData, frameWidth, frameHeight);
 * if (stats.isValid()) {
 *     // Use stats.contrast, stats.phaseError, etc.
 * }
 * \endcode
 */
class AfStatsCalculator {
public:
	/**
	 * \brief Calculation method
	 */
	enum class Method {
		Sobel,    ///< Sobel edge detection (default)
		Laplacian, ///< Laplacian edge detection
		Variance   ///< Variance-based contrast
	};

	/**
	 * \brief Initialize the calculator
	 */
	AfStatsCalculator();
	~AfStatsCalculator();

	/**
	 * \brief Set the calculation method
	 * \param method The method to use for contrast calculation
	 */
	void setMethod(Method method) { method_ = method; }

	/**
	 * \brief Set the region of interest
	 * \param roi The ROI to calculate statistics for
	 *
	 * If not set, the entire frame is used.
	 */
	void setRoi(const Rectangle &roi) { roi_ = roi; useRoi_ = true; }

	/**
	 * \brief Clear ROI to use entire frame
	 */
	void clearRoi() { useRoi_ = false; }

	/**
	 * \brief Add a weighted ROI
	 * \param roi The region
	 * \param weight The weight (0.0 to 1.0)
	 *
	 * Multiple ROIs can be added. The final contrast is a weighted average.
	 */
	void addWeightedRoi(const Rectangle &roi, float weight);

	/**
	 * \brief Clear all weighted ROIs
	 */
	void clearWeightedRois() { weightedRois_.clear(); }

	/**
	 * \brief Set minimum contrast threshold
	 * \param threshold Minimum valid contrast (0.0 to 1.0)
	 *
	 * Contrasts below this threshold are considered invalid.
	 */
	void setMinContrast(float threshold) { minContrast_ = threshold; }

	/**
	 * \brief Calculate AF statistics from raw frame data
	 * \param data Pointer to raw image data (8-bit or 16-bit)
	 * \param width Frame width in pixels
	 * \param height Frame height in pixels
	 * \param stride Stride in bytes (0 = no padding)
	 * \param bitsPerPixel Bits per pixel (8 or 16)
	 * \return AF statistics structure
	 *
	 * This is the main entry point. It calculates contrast metrics
	 * from the provided frame data.
	 */
	AfStats calculate(const uint8_t *data, uint32_t width, uint32_t height,
	                  uint32_t stride = 0, uint32_t bitsPerPixel = 8);

	/**
	 * \brief Calculate AF statistics from raw frame data (16-bit)
	 * \param data Pointer to 16-bit raw image data
	 * \param width Frame width in pixels
	 * \param height Frame height in pixels
	 * \param stride Stride in pixels (0 = no padding)
	 * \return AF statistics structure
	 */
	AfStats calculate16(const uint16_t *data, uint32_t width, uint32_t height,
	                    uint32_t stride = 0);

	/**
	 * \brief Calculate AF statistics from a Span
	 * \param data Span containing frame data
	 * \param width Frame width in pixels
	 * \param height Frame height in pixels
	 * \param bitsPerPixel Bits per pixel (8 or 16)
	 * \return AF statistics structure
	 */
	AfStats calculate(Span<const uint8_t> data, uint32_t width, uint32_t height,
	                  uint32_t bitsPerPixel = 8);

private:
	Method method_;
	Rectangle roi_;
	bool useRoi_;
	struct WeightedRoi {
		Rectangle roi;
		float weight;
	};
	std::vector<WeightedRoi> weightedRois_;
	float minContrast_;

	// Helper functions
	float calculateContrastSobel(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride);
	float calculateContrastLaplacian(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride);
	float calculateContrastVariance(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride);

	float normalizeContrast(float rawContrast, uint32_t pixelCount);
};

} // namespace libipa
} // namespace libcamera
