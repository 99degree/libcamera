/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * \file af_stats.cpp
 * \brief CPU-based Auto-Focus Statistics Calculator Implementation
 */
#include "af_stats.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <libcamera/base/log.h>

namespace libcamera {
LOG_DEFINE_CATEGORY(LibipaAfStats)

namespace libipa {

AfStatsCalculator::AfStatsCalculator()
	: method_(Method::Sobel), useRoi_(false), minContrast_(0.01f)
{
}

AfStatsCalculator::~AfStatsCalculator() = default;

void AfStatsCalculator::addWeightedRoi(const Rectangle &roi, float weight)
{
	weightedRois_.push_back({ roi, std::clamp(weight, 0.0f, 1.0f) });
}

float AfStatsCalculator::normalizeContrast(float rawContrast, uint32_t pixelCount)
{
	if (pixelCount == 0)
		return 0.0f;

	// Normalize based on expected maximum contrast
	// This is heuristic and may need tuning for different sensors
	float maxExpected = static_cast<float>(pixelCount) * 255.0f * 4.0f; // Sobel max
	float normalized = rawContrast / maxExpected;
	return std::clamp(normalized, 0.0f, 1.0f);
}

float AfStatsCalculator::calculateContrastSobel(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride)
{
	if (stride == 0)
		stride = width;

	uint64_t totalGradient = 0;
	uint32_t validPixels = 0;

	// Sobel kernels
	const int8_t gx[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
	const int8_t gy[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };

	// Process image with Sobel filter
	for (uint32_t y = 1; y < height - 1; y++) {
		for (uint32_t x = 1; x < width - 1; x++) {
			int32_t gradX = 0;
			int32_t gradY = 0;

			// Apply Sobel kernels
			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					uint32_t px = x + kx;
					uint32_t py = y + ky;
					uint8_t pixel = data[py * stride + px];

					gradX += pixel * gx[ky + 1][kx + 1];
					gradY += pixel * gy[ky + 1][kx + 1];
				}
			}

			// Gradient magnitude
			uint32_t magnitude = static_cast<uint32_t>(std::sqrt(gradX * gradX + gradY * gradY));
			totalGradient += magnitude;
			validPixels++;
		}
	}

	if (validPixels == 0)
		return 0.0f;

	return static_cast<float>(totalGradient) / validPixels;
}

float AfStatsCalculator::calculateContrastLaplacian(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride)
{
	if (stride == 0)
		stride = width;

	int64_t totalLaplacian = 0;
	uint32_t validPixels = 0;

	// Laplacian kernel
	const int8_t lap[3][3] = { {1, 1, 1}, {1, -8, 1}, {1, 1, 1} };

	for (uint32_t y = 1; y < height - 1; y++) {
		for (uint32_t x = 1; x < width - 1; x++) {
			int32_t laplacian = 0;

			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					uint32_t px = x + kx;
					uint32_t py = y + ky;
					uint8_t pixel = data[py * stride + px];

					laplacian += pixel * lap[ky + 1][kx + 1];
				}
			}

			totalLaplacian += std::abs(laplacian);
			validPixels++;
		}
	}

	if (validPixels == 0)
		return 0.0f;

	return static_cast<float>(totalLaplacian) / validPixels;
}

float AfStatsCalculator::calculateContrastVariance(const uint8_t *data, uint32_t width, uint32_t height, uint32_t stride)
{
	if (stride == 0)
		stride = width;

	uint64_t sum = 0;
	uint64_t sumSq = 0;
	uint32_t count = 0;

	for (uint32_t y = 0; y < height; y++) {
		for (uint32_t x = 0; x < width; x++) {
			uint8_t pixel = data[y * stride + x];
			sum += pixel;
			sumSq += static_cast<uint64_t>(pixel) * pixel;
			count++;
		}
	}

	if (count == 0)
		return 0.0f;

	double mean = static_cast<double>(sum) / count;
	double variance = (static_cast<double>(sumSq) / count) - (mean * mean);

	return static_cast<float>(variance);
}

AfStats AfStatsCalculator::calculate(const uint8_t *data, uint32_t width, uint32_t height,
                                     uint32_t stride, uint32_t bitsPerPixel)
{
	AfStats stats = {};

	if (!data || width == 0 || height == 0)
		return stats;

	if (bitsPerPixel == 16) {
		// For 16-bit data, we need to handle it differently
		// For now, convert to 8-bit by shifting
		const uint8_t *data8 = reinterpret_cast<const uint8_t *>(data);
		return calculate(data8, width, height, stride, 8);
	}

	if (stride == 0)
		stride = width;

	float rawContrast = 0.0f;
	uint32_t validPixels = 0;

	// Calculate contrast using the selected method
	switch (method_) {
	case Method::Sobel:
		rawContrast = calculateContrastSobel(data, width, height, stride);
		break;
	case Method::Laplacian:
		rawContrast = calculateContrastLaplacian(data, width, height, stride);
		break;
	case Method::Variance:
		rawContrast = calculateContrastVariance(data, width, height, stride);
		break;
	}

	// If using weighted ROIs, calculate contrast for each ROI
	if (!weightedRois_.empty()) {
		float weightedSum = 0.0f;
		float totalWeight = 0.0f;

		for (const auto &weightedRoi : weightedRois_) {
			const Rectangle &roi = weightedRoi.roi;
			float weight = weightedRoi.weight;

			// Ensure ROI is within bounds
			uint32_t x = std::min(roi.x, width - 1);
			uint32_t y = std::min(roi.y, height - 1);
			uint32_t w = std::min(roi.width, width - x);
			uint32_t h = std::min(roi.height, height - y);

			if (w > 2 && h > 2) {
				// Extract ROI data
				std::vector<uint8_t> roiData(w * h);
				for (uint32_t row = 0; row < h; row++) {
					memcpy(&roiData[row * w],
					       &data[(y + row) * stride + x],
					       w);
				}

				float roiContrast = 0.0f;
				switch (method_) {
				case Method::Sobel:
					roiContrast = calculateContrastSobel(roiData.data(), w, h, w);
					break;
				case Method::Laplacian:
					roiContrast = calculateContrastLaplacian(roiData.data(), w, h, w);
					break;
				case Method::Variance:
					roiContrast = calculateContrastVariance(roiData.data(), w, h, w);
					break;
				}

				weightedSum += roiContrast * weight;
				totalWeight += weight;
			}
		}

		if (totalWeight > 0) {
			rawContrast = weightedSum / totalWeight;
			validPixels = width * height; // Approximate
		}
	} else if (useRoi_) {
		// Calculate contrast for single ROI
		uint32_t x = std::min(roi_.x, width - 1);
		uint32_t y = std::min(roi_.y, height - 1);
		uint32_t w = std::min(roi_.width, width - x);
		uint32_t h = std::min(roi_.height, height - y);

		if (w > 2 && h > 2) {
			std::vector<uint8_t> roiData(w * h);
			for (uint32_t row = 0; row < h; row++) {
				memcpy(&roiData[row * w],
				       &data[(y + row) * stride + x],
				       w);
			}

			switch (method_) {
			case Method::Sobel:
				rawContrast = calculateContrastSobel(roiData.data(), w, h, w);
				break;
			case Method::Laplacian:
				rawContrast = calculateContrastLaplacian(roiData.data(), w, h, w);
				break;
			case Method::Variance:
				rawContrast = calculateContrastVariance(roiData.data(), w, h, w);
				break;
			}
			validPixels = w * h;
		}
	} else {
		// Full frame
		validPixels = width * height;
	}

	stats.rawContrast = rawContrast;
	stats.contrast = normalizeContrast(rawContrast, validPixels);
	stats.validPixels = validPixels;
	stats.phaseError = 0.0f; // No phase data from CPU-only calculation
	stats.phaseConfidence = 0.0f;

	// Check minimum contrast threshold
	if (stats.contrast < minContrast_) {
		// Very low contrast - might be out of range or bad lighting
		LOG(LibipaAfStats, Debug) << "Low contrast: " << stats.contrast;
	}

	return stats;
}

AfStats AfStatsCalculator::calculate16(const uint16_t *data, uint32_t width, uint32_t height,
                                       uint32_t stride)
{
	if (stride == 0)
		stride = width;

	AfStats stats = {};

	if (!data || width == 0 || height == 0)
		return stats;

	int64_t totalGradient = 0;
	uint32_t validPixels = 0;

	// Sobel kernels
	const int8_t gx[3][3] = { {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1} };
	const int8_t gy[3][3] = { {-1, -2, -1}, {0, 0, 0}, {1, 2, 1} };

	for (uint32_t y = 1; y < height - 1; y++) {
		for (uint32_t x = 1; x < width - 1; x++) {
			int32_t gradX = 0;
			int32_t gradY = 0;

			for (int ky = -1; ky <= 1; ky++) {
				for (int kx = -1; kx <= 1; kx++) {
					uint32_t px = x + kx;
					uint32_t py = y + ky;
					uint16_t pixel = data[py * stride + px];

					gradX += pixel * gx[ky + 1][kx + 1];
					gradY += pixel * gy[ky + 1][kx + 1];
				}
			}

			uint32_t magnitude = static_cast<uint32_t>(std::sqrt(gradX * gradX + gradY * gradY));
			totalGradient += magnitude;
			validPixels++;
		}
	}

	if (validPixels == 0)
		return stats;

	float rawContrast = static_cast<float>(totalGradient) / validPixels;
	stats.rawContrast = rawContrast;
	stats.contrast = normalizeContrast(rawContrast, validPixels);
	stats.validPixels = validPixels;
	stats.phaseError = 0.0f;
	stats.phaseConfidence = 0.0f;

	return stats;
}

AfStats AfStatsCalculator::calculate(Span<const uint8_t> data, uint32_t width, uint32_t height,
                                     uint32_t bitsPerPixel)
{
	if (data.size() < width * height * (bitsPerPixel / 8)) {
		LOG(LibipaAfStats, Error) << "Insufficient data size";
		return AfStats{};
	}

	return calculate(data.data(), width, height, 0, bitsPerPixel);
}

} // namespace libipa
} // namespace libcamera
