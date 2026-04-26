/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "onnx_engine_ort.h"
#include <libcamera/base/log.h>
#include <unistd.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(OnnxEngine)

namespace ipa::soft {

OnnxEngineOrt::OnnxEngineOrt()
{
	sessionOptions_.SetIntraOpNumThreads(2);
	sessionOptions_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}

OnnxEngineOrt::~OnnxEngineOrt()
{
	delete session_;
}

int OnnxEngineOrt::loadModel(const std::string &modelPath)
{
	if (access(modelPath.c_str(), R_OK) != 0) {
		LOG(OnnxEngine, Warning) << "Model not found: " << modelPath;
		return -ENOENT;
	}

	try {
		if (session_)
			delete session_;

		session_ = new Ort::Session(env_, modelPath.c_str(), sessionOptions_);

		Ort::AllocatorWithDefaultOptions allocator;
		size_t nIn = session_->GetInputCount();
		size_t nOut = session_->GetOutputCount();

		for (size_t i = 0; i < nIn; i++) {
			auto nm = session_->GetInputNameAllocated(i, allocator);
			inputNames_.push_back(strdup(nm.get()));
		}
		for (size_t i = 0; i < nOut; i++) {
			auto nm = session_->GetOutputNameAllocated(i, allocator);
			outputNames_.push_back(strdup(nm.get()));
		}

		LOG(OnnxEngine, Info) << "Loaded: " << modelPath
				      << " (" << nIn << " in, " << nOut << " out)";
		return 0;
	} catch (const std::exception &e) {
		LOG(OnnxEngine, Error) << "loadModel failed: " << e.what();
		return -EINVAL;
	}
}

int OnnxEngineOrt::runInference(const std::vector<float> &inputs,
				std::vector<float> &outputs)
{
	if (!session_)
		return -EINVAL;

	try {
		std::vector<int64_t> shape = {1, (int64_t)inputs.size()};

		auto inTensor = Ort::Value::CreateTensor<float>(
			memoryInfo_, const_cast<float *>(inputs.data()),
			inputs.size(), shape.data(), shape.size());

		std::vector<const char *> inPtrs, outPtrs;
		for (auto &n : inputNames_) inPtrs.push_back(n);
		for (auto &n : outputNames_) outPtrs.push_back(n);

		auto outTensors = session_->Run(Ort::RunOptions{nullptr},
						inPtrs.data(), &inTensor, inPtrs.size(),
						outPtrs.data(), outPtrs.size());

		outputs.clear();
		for (auto &t : outTensors) {
			float *d = t.GetTensorMutableData<float>();
			size_t n = t.GetTensorTypeAndShapeInfo().GetElementCount();
			outputs.insert(outputs.end(), d, d + n);
		}
		return 0;
	} catch (const std::exception &e) {
		LOG(OnnxEngine, Error) << "Inference failed: " << e.what();
		return -EINVAL;
	}
}

} // namespace ipa::soft
} // namespace libcamera