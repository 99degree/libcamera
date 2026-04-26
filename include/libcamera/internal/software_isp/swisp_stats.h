/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Linaro Ltd
 *
 * Statistics data format used by the software ISP and software IPA
 */
#pragma once

#include <array>
#include <stdint.h>

#include "libcamera/internal/vector.h"

namespace libcamera {

/**
 * \brief Struct that holds the statistics for the Software ISP
 *
 * The struct value types are large enough to not overflow.
 * Should they still overflow for some reason, no check is performed and they
 * wrap around.
 */
struct SwIspStats {
	/**
	 * \brief True if the statistics buffer contains valid data, false if
	 * no statistics were generated for this frame
	 */
	bool valid;

	/**
	 * \brief Sums of colour channels of all the sampled pixels
	 */
	RGB<uint64_t> sum_;

	/**
	 * \brief Number of bins in the yHistogram
	 */
	static constexpr unsigned int kYHistogramSize = 64;

	/**
	 * \brief Type of the histogram.
	 */
	using Histogram = std::array<uint32_t, kYHistogramSize>;

	/**
	 * \brief A histogram of luminance values of all the sampled pixels
	 */
	Histogram yHistogram;

	/**
	 * \brief AF contrast score (0.0 to 1.0, higher = sharper)
	 *
	 * Normalized contrast metric calculated from the frame.
	 * Higher values indicate a more in-focus image.
	 */
	float afContrast;

	/**
	 * \brief Raw AF contrast value (unnormalized)
	 *
	 * Useful for debugging and threshold tuning.
	 */
	float afRawContrast;

	/**
	 * \brief Number of pixels used in AF calculation
	 */
	uint32_t afValidPixels;

	/**
	 * \brief Phase detection error in pixels (if available)
	 *
	 * Positive value means lens is too close.
	 * Negative value means lens is too far.
	 * Zero means no phase data available.
	 */
	float afPhaseError;

	/**
	 * \brief Confidence in phase detection (0.0 to 1.0)
	 *
	 * Only valid if afPhaseError is non-zero.
	 */
	float afPhaseConfidence;
};

} /* namespace libcamera */
