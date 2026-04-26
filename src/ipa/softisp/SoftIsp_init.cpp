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
	LOG(SoftIsp, Info) << "[IPA] SoftIsp::init() - begin";
	impl_->imageWidth = 1920;
	impl_->imageHeight = 1080;
	impl_->initialized = true;

	LOG(SoftIsp, Info) << "[IPA] SoftIsp::init() - end (ready)";

	return 0;
}

void SoftIsp::ensureModelsLoaded()
{
	LOG(SoftIsp, Info) << "[IPA] ensureModelsLoaded - begin";

	const char *modelDir = getenv("SOFTISP_MODEL_DIR");
	if (!modelDir) {
		LOG(SoftIsp, Info) << "[IPA] ensureModelsLoaded - no SOFTISP_MODEL_DIR";
		return;
	}

	if (!impl_->algoEngine->isLoaded()) {
		std::string path = std::string(modelDir) + "/algo.onnx";
		LOG(SoftIsp, Info) << "[IPA] Loading algo model: " << path;
		impl_->algoEngine->loadModel(path);
	}

	if (!impl_->applierEngine->isLoaded()) {
		std::string path = std::string(modelDir) + "/applier.onnx";
		LOG(SoftIsp, Info) << "[IPA] Loading applier model: " << path;
		impl_->applierEngine->loadModel(path);
	}

	LOG(SoftIsp, Info) << "[IPA] ensureModelsLoaded - done";
}