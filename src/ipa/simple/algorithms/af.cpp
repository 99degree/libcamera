/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * \file af.cpp
 * \brief Auto-Focus IPA Algorithm Implementation for Simple Pipeline
 */
#include "af.h"

#include <libcamera/base/log.h>
#include <libcamera/control_ids.h>

#include "simple/ipa_interface.h"

namespace libcamera {
LOG_DEFINE_CATEGORY(IPASoftAf)

namespace ipa::soft::algorithms {

Af::Af()
	: initialized_(false), frameCount_(0), lensPositionUpdated_(false), lastLensPosition_(0)
{
	afAlgo_ = std::make_unique<libipa::AfAlgo>();
	statsCalculator_ = std::make_unique<libipa::AfStatsCalculator>();

	// Default configuration - can be overridden by config file
	afAlgo_->setRange(0.0f, 12.0f, 1.0f); // Dioptres
	afAlgo_->setSpeed(1.0f, 0.25f, 2.0f);
	afAlgo_->setMode(libipa::AfMode::Manual); // Start in manual mode

	// AF statistics configuration
	statsCalculator_->setMethod(libipa::AfStatsCalculator::Method::Sobel);
}

Af::~Af() = default;

int Af::configure(IPAContext &context, const IPAConfigInfo &configInfo)
{
	(void)configInfo;

	// Initialize AF algorithm
	afAlgo_->setRange(0.0f, 12.0f, 1.0f);

	// Try to load configuration from file
	// This can be customized based on the camera module
	std::string configPath = "/data/data/com.termux/files/home/libcamera/config/af_config.ini";
	afAlgo_->loadConfig(configPath);

	// Set up weighted ROIs for AF (center-weighted by default)
	Rectangle mainRoi(
		configInfo.pixelArraySize.width / 4,
		configInfo.pixelArraySize.height / 4,
		configInfo.pixelArraySize.width / 2,
		configInfo.pixelArraySize.height / 2
	);
	statsCalculator_->addWeightedRoi(mainRoi, 1.0f);

	// Add corner ROIs for better edge detection
	Rectangle cornerRoi1(0, 0, configInfo.pixelArraySize.width / 4, configInfo.pixelArraySize.height / 4);
	Rectangle cornerRoi2(configInfo.pixelArraySize.width * 3 / 4, 0,
	                     configInfo.pixelArraySize.width / 4, configInfo.pixelArraySize.height / 4);
	Rectangle cornerRoi3(0, configInfo.pixelArraySize.height * 3 / 4,
	                     configInfo.pixelArraySize.width / 4, configInfo.pixelArraySize.height / 4);
	Rectangle cornerRoi4(configInfo.pixelArraySize.width * 3 / 4,
	                     configInfo.pixelArraySize.height * 3 / 4,
	                     configInfo.pixelArraySize.width / 4, configInfo.pixelArraySize.height / 4);

	statsCalculator_->addWeightedRoi(cornerRoi1, 0.3f);
	statsCalculator_->addWeightedRoi(cornerRoi2, 0.3f);
	statsCalculator_->addWeightedRoi(cornerRoi3, 0.3f);
	statsCalculator_->addWeightedRoi(cornerRoi4, 0.3f);

	initialized_ = true;
	frameCount_ = 0;
	lensPositionUpdated_ = false;

	LOG(IPASoftAf, Info) << "AF algorithm configured";
	return 0;
}

void Af::prepare(IPAContext &context, [[maybe_unused]] const uint32_t frame,
                 IPAFrameContext &frameContext, [[maybe_unused]] DebayerParams *params)
{
	(void)context;
	(void)frameContext;

	// Prepare AF algorithm for this frame
	// Currently no preparation needed, but can be extended
}

void Af::process(IPAContext &context, [[maybe_unused]] const uint32_t frame,
                 IPAFrameContext &frameContext, const SwIspStats *stats,
                 ControlList &metadata)
{
	(void)context;
	(void)frameContext;

	if (!initialized_)
		return;

	frameCount_++;

	// Handle any AF controls that were set
	afAlgo_->handleControls(metadata);

	// Get current AF mode
	libipa::AfMode mode = afAlgo_->getMode();

	if (mode == libipa::AfMode::Auto || mode == libipa::AfMode::Continuous) {
		// Use the AF contrast from stats if available
		// The stats should contain afContrast calculated by SwStatsCpu or SoftISP
		float contrast = 0.0f;
		float phaseError = 0.0f;
		float phaseConfidence = 0.0f;

		if (stats && stats->valid) {
			contrast = stats->afContrast;
			phaseError = stats->afPhaseError;
			phaseConfidence = stats->afPhaseConfidence;

			// If no AF stats were calculated yet (afContrast == 0),
			// this means the stats provider hasn't been updated yet.
			// In that case, we can't do AF properly.
			if (contrast == 0.0f && stats->afValidPixels == 0) {
				LOG(IPASoftAf, Warning) << "No AF stats available yet";
				// Set a default low contrast to allow algorithm to run
				contrast = 0.01f;
			}
		}

		// Process with AfAlgo
		bool updated = afAlgo_->process(contrast, phaseError, phaseConfidence);

		if (updated && afAlgo_->hasNewPosition()) {
			int32_t lensPosition = afAlgo_->getLensPosition();

			// Calculate the actual dioptres range for the control
			float minPos = afAlgo_->getMinLensPosition();
			float maxPos = afAlgo_->getMaxLensPosition();
			float dioptres = afAlgo_->getTargetDioptres();

			// Map dioptres to normalized range (0.0 to 1.0)
			float normalizedPos = 0.0f;
			float range = maxPos - minPos;
			if (range > 0.001f) {
				normalizedPos = (dioptres - minPos) / range;
			}

			// Set the lens position control (normalized 0.0-1.0)
			metadata.set(controls::LensPosition, normalizedPos);

			lensPositionUpdated_ = true;
			lastLensPosition_ = lensPosition;

			LOG(IPASoftAf, Debug) << "Lens position updated: " << lensPosition
			                      << " (" << dioptres << "D)";
		}
	}

	// Report AF state in metadata
	int32_t afState = 0; // Inactive by default
	switch (afAlgo_->getState()) {
	case libipa::AfState::Idle:
		afState = 0; // ANDROID_CONTROL_AF_STATE_INACTIVE
		break;
	case libipa::AfState::Scanning:
		afState = 3; // ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN
		break;
	case libipa::AfState::Focusing:
		afState = 3; // ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN
		break;
	case libipa::AfState::Failed:
		afState = 6; // ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED (failed)
		break;
	}
	metadata.set(controls::AfState, afState);

	// Report current focus position
	if (lensPositionUpdated_) {
		float minPos = afAlgo_->getMinLensPosition();
		float maxPos = afAlgo_->getMaxLensPosition();
		float dioptres = afAlgo_->getTargetDioptres();
		float normalizedPos = 0.0f;
		float range = maxPos - minPos;
		if (range > 0.001f) {
			normalizedPos = (dioptres - minPos) / range;
		}
		metadata.set(controls::LensPosition, normalizedPos);
	}
}

REGISTER_IPA_ALGORITHM(Af, "Af")

} // namespace ipa::soft::algorithms
} // namespace libcamera
