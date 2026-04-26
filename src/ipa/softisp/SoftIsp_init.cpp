/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <cstdlib>
#include <unistd.h>

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
		LOG(SoftIsp, Info) << "[IPA] SOFTISP_MODEL_DIR not set, skipping ONNX";
		return;
	}

	/* Log model directory and file existence for debugging */
	LOG(SoftIsp, Info) << "[IPA] Model dir: " << modelDir;

	if (!impl_->algoEngine.isLoaded()) {
		std::string path = std::string(modelDir) + "/algo.onnx";
		if (access(path.c_str(), R_OK) == 0) {
			LOG(SoftIsp, Info) << "[IPA] Found: " << path;
			impl_->algoEngine.loadModel(path);
		} else {
			LOG(SoftIsp, Warning) << "[IPA] Not found: " << path;
		}
	}

	if (!impl_->applierEngine.isLoaded()) {
		std::string path = std::string(modelDir) + "/applier.onnx";
		if (access(path.c_str(), R_OK) == 0) {
			LOG(SoftIsp, Info) << "[IPA] Found: " << path;
			impl_->applierEngine.loadModel(path);
		} else {
			LOG(SoftIsp, Warning) << "[IPA] Not found: " << path;
		}
	}

	LOG(SoftIsp, Info) << "[IPA] ensureModelsLoaded - done";
}