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
	if (impl_->algoEngine.isLoaded()) {
		LOG(SoftIsp, Info) << "[IPA] ensureModelsLoaded - already loaded";
		return;
	}

	const char *modelDir = getenv("SOFTISP_MODEL_DIR");
	if (!modelDir) {
		LOG(SoftIsp, Info) << "[IPA] ensureModelsLoaded - no SOFTISP_MODEL_DIR";
		return;
	}

	std::string algoModel = std::string(modelDir) + "/algo.onnx";
	std::string applierModel = std::string(modelDir) + "/applier.onnx";
	LOG(SoftIsp, Info) << "[IPA] ensureModelsLoaded - loading: " << algoModel;
	impl_->algoEngine.loadModel(algoModel);
	LOG(SoftIsp, Info) << "[IPA] ensureModelsLoaded - loading: " << applierModel;
	impl_->applierEngine.loadModel(applierModel);
	LOG(SoftIsp, Info) << "[IPA] ensureModelsLoaded - done";
}