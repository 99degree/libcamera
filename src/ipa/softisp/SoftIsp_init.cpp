/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstdlib>

int32_t SoftIsp::init(const libcamera::ipa::soft::IPASettings & /*settings*/,
		      const libcamera::SharedFD &fdStats,
		      const libcamera::SharedFD &fdParams,
		      const libcamera::ipa::soft::IPACameraSensorInfo &sensorInfo,
		      const libcamera::ControlInfoMap & /*sensorControls*/,
		      libcamera::ControlInfoMap * /*ipaControls*/,
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
		return -EINVAL;
	}

	std::string algoPath = std::string(modelDir) + "/algo.onnx";
	std::string applierPath = std::string(modelDir) + "/applier.onnx";

	int ret = impl_->algoEngine.loadModel(algoPath);
	if (ret < 0) {
		return ret;
	}

	ret = impl_->applierEngine.loadModel(applierPath);
	if (ret < 0) {
		return ret;
	}

	impl_->initialized = true;


	return 0;
}
