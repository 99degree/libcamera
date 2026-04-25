/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstdlib>

int32_t SoftIsp::init(const IPASettings & /*settings*/,
		      const libcamera::SharedFD & /*fdStats*/,
		      const libcamera::SharedFD & /*fdParams*/,
		      const IPACameraSensorInfo & /*sensorInfo*/,
		      const libcamera::ControlInfoMap & /*sensorControls*/,
		      libcamera::ControlInfoMap * /*ipaControls*/,
		      bool * /*ccmEnabled*/)
{
	// Use default resolution
	impl_->imageWidth = 1920;
	impl_->imageHeight = 1080;

	const char *modelDir = getenv("SOFTISP_MODEL_DIR");
	if (!modelDir) {
		LOG(IPASoftISP, Error) << "SOFTISP_MODEL_DIR not set";
		return -EINVAL;
	}

	// Load ONNX models
	std::string algoModel = std::string(modelDir) + "/algo.onnx";
	std::string applierModel = std::string(modelDir) + "/applier.onnx";

	if (impl_->algoEngine.loadModel(algoModel) < 0) {
		LOG(IPASoftISP, Error) << "Failed to load algo model: " << algoModel;
		return -EINVAL;
	}

	if (impl_->applierEngine.loadModel(applierModel) < 0) {
		LOG(IPASoftISP, Error) << "Failed to load applier model: " << applierModel;
		return -EINVAL;
	}

	impl_->initialized = true;
	LOG(IPASoftISP, Info) << "SoftISP initialized: " << impl_->imageWidth << "x" << impl_->imageHeight;

	return 0;
}
