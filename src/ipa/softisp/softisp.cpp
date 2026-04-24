/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - ONNX-based Image Processing Algorithm.
 */
#include "softisp.h"
#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>
#include <libcamera/controls.h>
#include <cstdlib>

namespace libcamera {
LOG_DEFINE_CATEGORY(SoftIsp)

namespace ipa::soft {

SoftIsp::SoftIsp() : impl_(std::make_unique<Impl>())
{
	LOG(SoftIsp, Info) << "SoftIsp created";
}

SoftIsp::~SoftIsp() = default;

int32_t SoftIsp::init(const IPASettings & /*settings*/,
		      const SharedFD & /*fdStats*/,
		      const SharedFD & /*fdParams*/,
		      const IPACameraSensorInfo &sensorInfo,
		      const ControlInfoMap & /*sensorControls*/,
		      ControlInfoMap * /*ipaControls*/,
		      bool * /*ccmEnabled*/)
{
	impl_->imageWidth = sensorInfo.outputSize.width;
	impl_->imageHeight = sensorInfo.outputSize.height;

	// Get model directory from environment
	const char *modelDir = getenv("SOFTISP_MODEL_DIR");
	if (!modelDir) {
		LOG(SoftIsp, Error) << "SOFTISP_MODEL_DIR environment variable not set";
		return -EINVAL;
	}

	std::string algoPath = std::string(modelDir) + "/algo.onnx";
	std::string applierPath = std::string(modelDir) + "/applier.onnx";

	// Load algo model
	int ret = impl_->algoEngine.loadModel(algoPath);
	if (ret < 0) {
		LOG(SoftIsp, Error) << "Failed to load algo model: " << algoPath;
		return ret;
	}

	// Load applier model
	ret = impl_->applierEngine.loadModel(applierPath);
	if (ret < 0) {
		LOG(SoftIsp, Error) << "Failed to load applier model: " << applierPath;
		return ret;
	}

	impl_->initialized = true;

	LOG(SoftIsp, Info) << "SoftISP initialized for "
			   << impl_->imageWidth << "x" << impl_->imageHeight;
	LOG(SoftIsp, Info) << "Models loaded from: " << modelDir;

	return 0;
}

int32_t SoftIsp::start()
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Error) << "Not initialized";
		return -ENODEV;
	}

	LOG(SoftIsp, Info) << "SoftISP started";
	return 0;
}

void SoftIsp::stop()
{
	LOG(SoftIsp, Info) << "SoftISP stopped";
}

int32_t SoftIsp::configure(const IPAConfigInfo & /*configInfo*/)
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Error) << "Not initialized";
		return -ENODEV;
	}

	LOG(SoftIsp, Info) << "SoftISP configured";
	return 0;
}

void SoftIsp::queueRequest(const uint32_t frame, const ControlList & /*sensorControls*/)
{
	LOG(SoftIsp, Debug) << "queueRequest: frame=" << frame;
}

void SoftIsp::computeParams(const uint32_t frame)
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "Not initialized";
		return;
	}

	LOG(SoftIsp, Debug) << "computeParams: frame=" << frame;
	// TODO: Use algoEngine to compute parameters if needed
}

void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId,
			   const ControlList & /*sensorControls*/)
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "Not initialized";
		return;
	}

	LOG(SoftIsp, Debug) << "processStats: frame=" << frame << ", buffer=" << bufferId;

	// Create metadata to return via signal
	ControlList metadata;

	// TODO: Populate metadata with ONNX inference results

	// Emit metadata via signal (this is how results are returned)
	metadataReady.emit(frame, metadata);
	// metadata.set(controls::AeState, controls::AeStateConverged);


	// TODO: Prepare input from stats and run algoEngine
	// For now, just log
	// std::vector<float> inputs = ...;
	// std::vector<float> outputs;
	// impl_->algoEngine.runInference(inputs, outputs);
	// impl_->algoOutput = outputs;
	// Populate sensorControls from outputs
}

void SoftIsp::processFrame(const uint32_t frame, const uint32_t bufferId,
			   const SharedFD & /*bufferFd*/, const int32_t /*planeIndex*/,
			   const int32_t /*width*/, const int32_t /*height*/,
			   const ControlList & /*results*/)
{
	(void)frame;
	(void)bufferId;

	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "Not initialized";
		return;
	}

	LOG(SoftIsp, Debug) << "processFrame: frame=" << frame << ", buffer=" << bufferId;

	// TODO: Prepare input (frame + algoOutput) and run applierEngine
	// std::vector<float> inputs = ...;
	// std::vector<float> outputs;
	// impl_->applierEngine.runInference(inputs, outputs);
	// Write outputs to buffer
}

std::string SoftIsp::logPrefix() const
{
	return "SoftIsp";
}

} /* namespace soft */
} /* namespace libcamera */
