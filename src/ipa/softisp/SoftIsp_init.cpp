/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstdlib>

int32_t SoftIsp::init(const IPASettings & /*settings*/,
		      const SharedFD &fdStats,
		      const SharedFD &fdParams,
		      const IPACameraSensorInfo &sensorInfo,
		      const ControlInfoMap & /*sensorControls*/,
		      ControlInfoMap * /*ipaControls*/,
		      bool * /*ccmEnabled*/)
{
	impl_->imageWidth = sensorInfo.outputSize.width;
	impl_->imageHeight = sensorInfo.outputSize.height;

	// Store FDs for later use in processStats/processFrame
	// These are kept alive by SharedFD reference counting
	// TODO: Store fdStats_ and fdParams_ in Impl struct
	// For now, we'll access them via the IPA interface if needed

	const char *modelDir = getenv("SOFTISP_MODEL_DIR");
	if (!modelDir) {
		LOG(SoftIsp, Error) << "SOFTISP_MODEL_DIR environment variable not set";
		return -EINVAL;
	}

	std::string algoPath = std::string(modelDir) + "/algo.onnx";
	std::string applierPath = std::string(modelDir) + "/applier.onnx";

	int ret = impl_->algoEngine.loadModel(algoPath);
	if (ret < 0) {
		LOG(SoftIsp, Error) << "Failed to load algo model: " << algoPath;
		return ret;
	}

	ret = impl_->applierEngine.loadModel(applierPath);
	if (ret < 0) {
		LOG(SoftIsp, Error) << "Failed to load applier model: " << applierPath;
		return ret;
	}

	impl_->initialized = true;

	LOG(SoftIsp, Info) << "SoftISP initialized for "
			   << impl_->imageWidth << "x" << impl_->imageHeight;
	LOG(SoftIsp, Info) << "Models loaded from: " << modelDir;
	LOG(SoftIsp, Info) << "  - algo.onnx: AWB/AE computation";
	LOG(SoftIsp, Info) << "  - applier.onnx: Bayer → RGB/YUV";

	return 0;
}
