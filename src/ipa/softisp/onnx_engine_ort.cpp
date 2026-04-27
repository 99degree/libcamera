/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "onnx_engine.h"
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
		if (session_) delete session_;
		session_ = new Ort::Session(env_, modelPath.c_str(), sessionOptions_);
		Ort::AllocatorWithDefaultOptions allocator;

		// Get input information
		size_t nIn = session_->GetInputCount();
		for (size_t i = 0; i < nIn; i++) {
			auto nm = session_->GetInputNameAllocated(i, allocator);
			inputNames_.push_back(strdup(nm.get()));

			// Get input type and shape
			Ort::TypeInfo typeInfo = session_->GetInputTypeInfo(i);
			const auto &tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();

			TensorInfo info;
			info.shape = tensorInfo.GetShape();
			info.type = tensorInfo.GetElementType();
			info.elementCount = tensorInfo.GetElementCount();
			inputInfo_[nm.get()] = info;
		}

		// Get output information
		size_t nOut = session_->GetOutputCount();
		for (size_t i = 0; i < nOut; i++) {
			auto nm = session_->GetOutputNameAllocated(i, allocator);
			outputNames_.push_back(strdup(nm.get()));

			// Get output type and shape
			Ort::TypeInfo typeInfo = session_->GetOutputTypeInfo(i);
			const auto &tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();

			TensorInfo info;
			info.shape = tensorInfo.GetShape();
			info.type = tensorInfo.GetElementType();
			info.elementCount = tensorInfo.GetElementCount();
			outputInfo_[nm.get()] = info;
		}

		LOG(OnnxEngine, Info) << "Loaded: " << modelPath << " (" << nIn << " in, " << nOut << " out)";
		return 0;
	} catch (const std::exception &e) {
		LOG(OnnxEngine, Error) << "loadModel failed: " << e.what();
		return -EINVAL;
	}
}

int OnnxEngineOrt::runInference(const std::vector<float> &inputs,
				   std::vector<float> &outputs)
{
	if (!session_) return -EINVAL;

	try {
		// Get input info
		std::vector<Ort::Value> inputTensors;
		std::vector<const char *> inPtrs;
		size_t offset = 0;

		for (const auto &inputName : inputNames_) {
			const auto &info = inputInfo_[inputName];
			
			// Check if we have enough input data
			size_t count = info.elementCount;
			if (offset + count > inputs.size()) {
				LOG(OnnxEngine, Error) << "Input data too small, expected "
					<< (offset + count) << " but got " << inputs.size();
				return -EINVAL;
			}
			
			// Create tensor
			auto tensor = Ort::Value::CreateTensor<float>(
				memoryInfo_,
				const_cast<float *>(inputs.data() + offset),
				count,
				info.shape.data(),
				info.shape.size());
			
			inputTensors.push_back(std::move(tensor));
			inPtrs.push_back(inputName);
			offset += count;
		}

		// Prepare output names
		std::vector<const char *> outPtrs;
		for (auto &n : outputNames_)
			outPtrs.push_back(n);

		// Run inference
		auto outTensors = session_->Run(
			Ort::RunOptions{nullptr},
			inPtrs.data(),
			inputTensors.data(),
			inPtrs.size(),
			outPtrs.data(),
			outPtrs.size());

		// Process outputs
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