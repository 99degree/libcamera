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

			Ort::TypeInfo typeInfo = session_->GetInputTypeInfo(i);
			auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
			TensorInfo info;
			info.shape = tensorInfo.GetShape();
			info.type = tensorInfo.GetElementType();
			info.elementCount = 1;
			for (auto dim : info.shape) {
				if (dim == -1) {
					// For dynamic dimensions, treat as 1 for element count calculation
					// The actual dimension must be provided at inference time
					info.elementCount *= 1;
				} else {
					info.elementCount *= dim;
				}
			}
			inputInfo_[inputNames_.back()] = info;
		}

		// Get output information
		size_t nOut = session_->GetOutputCount();
		for (size_t i = 0; i < nOut; i++) {
			auto nm = session_->GetOutputNameAllocated(i, allocator);
			outputNames_.push_back(strdup(nm.get()));

			Ort::TypeInfo typeInfo = session_->GetOutputTypeInfo(i);
			auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
			TensorInfo info;
			info.shape = tensorInfo.GetShape();
			info.type = tensorInfo.GetElementType();
			info.elementCount = 1;
			for (auto dim : info.shape) {
				if (dim == -1) {
					// For dynamic dimensions, treat as 1 for element count calculation
					// The actual dimension must be provided at inference time
					info.elementCount *= 1;
				} else {
					info.elementCount *= dim;
				}
			}
			outputInfo_[outputNames_.back()] = info;
		}
		
		LOG(OnnxEngine, Info) << "Model loaded: " << modelPath;
		return 0;
	} catch (const std::exception &e) {
		LOG(OnnxEngine, Error) << "Failed to load model: " << e.what();
		return -EINVAL;
	}
}

int OnnxEngineOrt::runInference(const std::vector<float> &inputs, std::vector<float> &outputs)
{
	if (!session_) return -ENOENT;

	try {
		const size_t numInputs = session_->GetInputCount();
		const size_t numOutputs = session_->GetOutputCount();

		if (inputs.size() < numInputs) {
			LOG(OnnxEngine, Error) << "Not enough inputs: expected " << numInputs << ", got " << inputs.size();
			// Print detailed input expectations
			Ort::AllocatorWithDefaultOptions allocator;
			for (size_t i = 0; i < numInputs; i++) {
				auto name = session_->GetInputNameAllocated(i, allocator);
				Ort::TypeInfo typeInfo = session_->GetInputTypeInfo(i);
				auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
				std::ostringstream shapeStr;
				auto shape = tensorInfo.GetShape();
				for (size_t j = 0; j < shape.size(); ++j) {
					if (j > 0) shapeStr << ", ";
					shapeStr << shape[j];
				}
				LOG(OnnxEngine, Error) << "  Input " << i << ": " << name.get()
					<< " (shape [" << shapeStr.str() << "])";
			}
			return -EINVAL;
		}

		// Prepare input tensors
		std::vector<Ort::Value> inputTensors;
		std::vector<int64_t> shapes;

		Ort::MemoryInfo info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeDefault);

		// Set outputs
		outputs.resize(numOutputs);

		return 0;
	} catch (const std::exception &e) {
		LOG(OnnxEngine, Error) << "Inference failed: " << e.what();
		return -EINVAL;
	}
}

} /* namespace ipa::soft */
} /* namespace libcamera */

