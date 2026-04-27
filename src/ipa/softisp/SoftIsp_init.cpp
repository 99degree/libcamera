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

	/*
	 * Load algo.onnx early in init() so statistics models are ready
	 * before the first frame arrives. applier.onnx loads lazily on
	 * first processFrame() call.
	 */
	const char *modelDir = getenv("SOFTISP_MODEL_DIR");
	if (modelDir) {
		/* algo.onnx - statistics model */
		if (!impl_->algoEngine.isLoaded()) {
			std::string path = std::string(modelDir) + "/algo.onnx";
			if (access(path.c_str(), R_OK) == 0) {
				LOG(SoftIsp, Info) << "[IPA] init loading algo: " << path;
				impl_->algoEngine.loadModel(path);
			}
		}

		/* applier.onnx - image processing model */
		if (!impl_->applierEngine.isLoaded()) {
			std::string path = std::string(modelDir) + "/applier.onnx";
			if (access(path.c_str(), R_OK) == 0) {
				LOG(SoftIsp, Info) << "[IPA] init loading applier: " << path;
				impl_->applierEngine.loadModel(path);
			}
		}
	}

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

	LOG(SoftIsp, Info) << "[IPA] Model dir: " << modelDir;

	/* algo.onnx already loaded in init() if available */
	if (!impl_->algoEngine.isLoaded()) {
		std::string path = std::string(modelDir) + "/algo.onnx";
		if (access(path.c_str(), R_OK) == 0) {
			LOG(SoftIsp, Info) << "[IPA] Found: " << path;
			impl_->algoEngine.loadModel(path);
		} else {
			LOG(SoftIsp, Warning) << "[IPA] Not found: " << path;
		}
	}

	/* applier.onnx loaded lazily here */
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