/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - implementation of the ONNX-based Image Processing Algorithm.
 */

#include "softisp.h"

#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>
#include <onnxruntime_cxx_api.h>
#include <unistd.h>

#include <algorithm>
#include <vector>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftIsp)

namespace ipa::soft {

/* -----------------------------------------------------------------
 * SoftIsp::Impl - Private implementation holding ONNX sessions.
 * ----------------------------------------------------------------- */
struct SoftIsp::Impl {
	Impl() = default;
	~Impl() = default;

	// ONNX Runtime environment and sessions
	Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "SoftIsp"};
	Ort::SessionOptions sessionOptions;
	std::unique_ptr<Ort::Session> algoSession;
	std::unique_ptr<Ort::Session> applierSession;
	Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
		Ort::AllocatorWithDefaultOptions allocator;
	bool initialized = false;
	int imageWidth = 640;
	int imageHeight = 480;

	// Model paths
	std::string algoModelPath;
	std::string applierModelPath;

	// Input/Output names
	std::vector<const char*> algoInputNames;
	std::vector<const char*> algoOutputNames;
	std::vector<const char*> applierInputNames;
	std::vector<const char*> applierOutputNames;
};

/* -----------------------------------------------------------------
 * SoftIsp - Public Implementation
 * ----------------------------------------------------------------- */

SoftIsp::SoftIsp() : impl_(std::make_unique<Impl>())
{
}

SoftIsp::~SoftIsp() = default;

std::string SoftIsp::logPrefix() const
{
	return "SoftIsp";
}

int32_t SoftIsp::init(const IPASettings &settings, const SharedFD &fdStats, const SharedFD &fdParams,
		     const IPACameraSensorInfo &sensorInfo, const ControlInfoMap &sensorControls,
		     ControlInfoMap *ipaControls, bool *ccmEnabled)
{
	LOG(SoftIsp, Info) << "Initializing SoftISP algorithm";

	/* Get model directory from environment variable */
	const char *modelDir = getenv("SOFTISP_MODEL_DIR");
	if (!modelDir) {
		LOG(SoftIsp, Error) << "SOFTISP_MODEL_DIR environment variable not set";
		return -EINVAL;
	}

	std::string algoPath = std::string(modelDir) + "/algo.onnx";
	std::string applierPath = std::string(modelDir) + "/applier.onnx";

	LOG(SoftIsp, Info) << "Looking for algo.onnx at " << algoPath;
	if (access(algoPath.c_str(), R_OK) != 0) {
		LOG(SoftIsp, Error) << "algo.onnx not found at " << algoPath;
		return -ENOENT;
	}
	LOG(SoftIsp, Info) << "Loading algo.onnx...";
	try {
		impl_->algoSession = std::make_unique<Ort::Session>(impl_->env, algoPath.c_str(), impl_->sessionOptions);
		LOG(SoftIsp, Info) << "algo.onnx loaded successfully";
	} catch (const Ort::Exception &e) {
		LOG(SoftIsp, Error) << "Failed to load algo.onnx: " << e.what();
		return -EINVAL;
	}

	LOG(SoftIsp, Info) << "Looking for applier.onnx at " << applierPath;
	if (access(applierPath.c_str(), R_OK) != 0) {
		LOG(SoftIsp, Error) << "applier.onnx not found at " << applierPath;
		return -ENOENT;
	}
	LOG(SoftIsp, Info) << "Loading applier.onnx...";
	try {
		impl_->applierSession = std::make_unique<Ort::Session>(impl_->env, applierPath.c_str(), impl_->sessionOptions);
		LOG(SoftIsp, Info) << "applier.onnx loaded successfully";
	} catch (const Ort::Exception &e) {
		LOG(SoftIsp, Error) << "Failed to load applier.onnx: " << e.what();
		return -EINVAL;
	}
	fprintf(stderr, ">>> APPLIER LOADED, setting ccmEnabled\n");
	fflush(stderr);

	if (ccmEnabled) *ccmEnabled = true;
	fprintf(stderr, ">>> SoftISP initialization complete (models detected)\n"); fflush(stderr);
	fprintf(stderr, ">>> About to return from init()\n");
	fflush(stderr);
	return 0;
}

int32_t SoftIsp::start()
{
	LOG(SoftIsp, Info) << "SoftISP algorithm started";
	return 0;
}

void SoftIsp::stop()
{
	LOG(SoftIsp, Info) << "SoftISP algorithm stopped";
}

int32_t SoftIsp::configure(const IPAConfigInfo &configInfo)
{
	LOG(SoftIsp, Info) << "Configuring SoftISP algorithm";
	// TODO: Parse configInfo and set up ONNX input shapes
	return 0;
}

void SoftIsp::queueRequest(const uint32_t frame, const ControlList &sensorControls)
{
	LOG(SoftIsp, Debug) << "Queueing request for frame " << frame;
	// TODO: Queue the request for processing
}

void SoftIsp::computeParams(const uint32_t frame)
{
	// Optional: Compute parameters if needed
}

void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId,
			     const ControlList &sensorControls)
{
	LOG(SoftIsp, Info) << ">>> processStats called for frame " << frame;
	LOG(SoftIsp, Info) << "  initialized=" << impl_->initialized 
			     << " algoSession=" << (impl_->algoSession ? "yes" : "no")
			     << " applierSession=" << (impl_->applierSession ? "yes" : "no");
	if (!impl_->initialized || !impl_->algoSession || !impl_->applierSession) {
		LOG(SoftIsp, Error) << "SoftISP not properly initialized";
		return;
	}

	LOG(SoftIsp, Info) << "Processing frame " << frame << " buffer " << bufferId;

	/* Check if models are loaded */
	if (!impl_->algoSession || !impl_->applierSession) {
		LOG(SoftIsp, Error) << "ONNX models not loaded, skipping inference";
		return;
	}

	/*
	 * TODO: Implement actual inference pipeline:

 * Model Structure (verified via softisp-onnx-test):
 * - algo.onnx: 4 inputs, 15 outputs
 *   Inputs: image_desc.input.image.function, image_desc.input.width.function,
 *           image_desc.input.frame_id.function, blacklevel.offset.function
 * - applier.onnx: 10 inputs, 7 outputs
 *   Inputs: 4 original + 6 coefficient tensors from algo.onnx
	 * 1. Extract statistics from the frame buffer (e.g., histogram, AWB stats)
	 * 2. Prepare input tensors for algo.onnx
	 * 3. Run algo.onnx inference -> get ISP coefficients
	 * 4. Prepare input tensors for applier.onnx (coefficients + image data)
	 * 5. Run applier.onnx inference -> get processed image parameters
	 * 6. Apply parameters to the frame buffer
	 *
	 * For now, we log the frame processing step.
	 */
	LOG(SoftIsp, Info) << "Frame " << frame << " processed (inference logic to be implemented)";
}

} // namespace ipa::soft
} // namespace libcamera
