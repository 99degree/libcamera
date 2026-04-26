/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstdlib>

int32_t SoftIsp::init(const IPASettings & /*settings*/,
		      const SharedFD & /*fdStats*/,
		      const SharedFD & /*fdParams*/,
		      const IPACameraSensorInfo & /*sensorInfo*/,
		      const ControlInfoMap & /*sensorControls*/,
		      ControlInfoMap * /*ipaControls*/,
		      bool * /*ccmEnabled*/)
{
	impl_->imageWidth = 1920;
	impl_->imageHeight = 1080;

#if 0
	const char *modelDir = getenv("SOFTISP_MODEL_DIR");
	if (modelDir) {
		std::string algoModel = std::string(modelDir) + "/algo.onnx";
		std::string applierModel = std::string(modelDir) + "/applier.onnx";
		impl_->algoEngine.loadModel(algoModel);
		impl_->applierEngine.loadModel(applierModel);
	}
#endif

	impl_->initialized = true;
	LOG(SoftIsp, Info) << "SoftISP initialized: " << impl_->imageWidth << "x" << impl_->imageHeight;

	return 0;
}