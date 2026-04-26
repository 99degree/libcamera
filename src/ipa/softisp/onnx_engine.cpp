/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "onnx_engine.h"
#include <libcamera/base/log.h>
#include <algorithm>
#include <unistd.h>

namespace libcamera {
LOG_DEFINE_CATEGORY(OnnxEngine)

namespace ipa::soft {

OnnxEngine::OnnxEngine()
    : env_(nullptr),
      memoryInfo_(nullptr)
{
	LOG(OnnxEngine, Info) << "[ONNX] Constructor: begin";
	sessionOptions_.SetIntraOpNumThreads(2);
	sessionOptions_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
	LOG(OnnxEngine, Info) << "[ONNX] Constructor: end";
}

OnnxEngine::~OnnxEngine()
{
	LOG(OnnxEngine, Info) << "[ONNX] Destructor: begin";
	if (session_) {
		delete session_;
		session_ = nullptr;
	}
	if (memoryInfo_) {
		delete static_cast<Ort::MemoryInfo *>(memoryInfo_);
		memoryInfo_ = nullptr;
	}
	if (env_) {
		delete static_cast<Ort::Env *>(env_);
		env_ = nullptr;
	}
	LOG(OnnxEngine, Info) << "[ONNX] Destructor: end";
}

static bool initOrt(Ort::Env *&env, Ort::MemoryInfo *&memInfo)
{
	if (env)
		return true;

	LOG(OnnxEngine, Info) << "[ONNX] initOrt: creating Env...";
	try {
		env = new Ort::Env(ORT_LOGGING_LEVEL_WARNING, "SoftIsp");
		LOG(OnnxEngine, Info) << "[ONNX] initOrt: Env created, creating MemoryInfo...";
		memInfo = new Ort::MemoryInfo(
			Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
		LOG(OnnxEngine, Info) << "[ONNX] initOrt: MemoryInfo created";
		return true;
	} catch (const std::exception &e) {
		LOG(OnnxEngine, Error) << "[ONNX] initOrt: exception: " << e.what();
		return false;
	}
}

int OnnxEngine::loadModel(const std::string &modelPath)
{
	LOG(OnnxEngine, Info) << "[ONNX] loadModel: " << modelPath;

	if (!initOrt(env_, memoryInfo_)) {
		LOG(OnnxEngine, Error) << "[ONNX] loadModel: initOrt failed";
		return -EINVAL;
	}

	if (access(modelPath.c_str(), R_OK) != 0) {
		LOG(OnnxEngine, Warning) << "[ONNX] loadModel: file not found: " << modelPath;
		return -ENOENT;
	}

	try {
		LOG(OnnxEngine, Info) << "[ONNX] loadModel: creating session...";
		if (session_) {
			delete session_;
			session_ = nullptr;
		}

		auto *s = new Ort::Session(*env_, modelPath.c_str(), sessionOptions_);
		session_ = s;
		LOG(OnnxEngine, Info) << "[ONNX] loadModel: session created";

		Ort::AllocatorWithDefaultOptions allocator;
		size_t numInputs = session_->GetInputCount();

		for (size_t i = 0; i < numInputs; i++) {
			auto inputName = session_->GetInputNameAllocated(i, allocator);
			inputNames_.push_back(strdup(inputName.get()));

			auto typeInfo = session_->GetInputTypeInfo(i);
			auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
			auto shape = tensorInfo.GetShape();
			auto elementType = tensorInfo.GetElementType();

			TensorInfo info;
			info.shape = shape;
			info.type = elementType;
			info.elementCount = 1;
			for (auto dim : shape)
				info.elementCount *= dim;

			inputInfo_[inputName.get()] = info;
		}

		size_t numOutputs = session_->GetOutputCount();
		for (size_t i = 0; i < numOutputs; i++) {
			auto outputName = session_->GetOutputNameAllocated(i, allocator);
			outputNames_.push_back(strdup(outputName.get()));

			auto typeInfo = session_->GetOutputTypeInfo(i);
			auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
			auto shape = tensorInfo.GetShape();
			auto elementType = tensorInfo.GetElementType();

			TensorInfo info;
			info.shape = shape;
			info.type = elementType;
			info.elementCount = 1;
			for (auto dim : shape)
				info.elementCount *= dim;

			outputInfo_[outputName.get()] = info;
		}

		LOG(OnnxEngine, Info) << "[ONNX] loadModel: done: " << numInputs << " inputs, " << numOutputs << " outputs";
		return 0;
	} catch (const std::exception &e) {
		LOG(OnnxEngine, Error) << "[ONNX] loadModel: exception: " << e.what();
		return -EINVAL;
	}
}

int OnnxEngine::runInference(const std::vector<float> &inputs,
                             std::vector<float> &outputs)
{
	LOG(OnnxEngine, Info) << "[ONNX] runInference: begin";

	if (!env_ || !session_ || !memoryInfo_) {
		LOG(OnnxEngine, Error) << "[ONNX] runInference: not initialized";
		return -EINVAL;
	}

	try {
		Ort::AllocatorWithDefaultOptions allocator;

		std::vector<Ort::Value> inputTensors;
		std::vector<int64_t> inputShape;
		std::vector<const char*> inputNames;
		size_t inputOffset = 0;

		for (auto &[name, info] : inputInfo_) {
			std::vector<float> inputData;

			if (inputs.empty()) {
				inputData.resize(info.elementCount, 0.0f);
			} else if (inputOffset + info.elementCount <= inputs.size()) {
				inputData.insert(inputData.end(),
						 inputs.begin() + inputOffset,
						 inputs.begin() + inputOffset + info.elementCount);
				inputOffset += info.elementCount;
			} else {
				LOG(OnnxEngine, Warning) << "[ONNX] runInference: not enough input data";
				return -EINVAL;
			}

			inputShape = info.shape;
			inputNames.push_back(name.c_str());

			inputTensors.push_back(Ort::Value::CreateTensor<float>(
				*memoryInfo_, inputData.data(), inputData.size(),
				inputShape.data(), inputShape.size()));
		}

		if (inputTensors.empty()) {
			LOG(OnnxEngine, Warning) << "[ONNX] runInference: no input tensors";
			return -EINVAL;
		}

		LOG(OnnxEngine, Info) << "[ONNX] runInference: running...";
		std::vector<const char*> outputNames;
		for (auto &name : outputNames_)
			outputNames.push_back(name);

		auto outputTensors = session_->Run(Ort::RunOptions{nullptr},
						   inputNames.data(),
						   inputTensors.data(),
						   inputTensors.size(),
						   outputNames.data(),
						   outputNames.size());

		outputs.clear();
		for (size_t i = 0; i < outputTensors.size(); i++) {
			float *outputData = outputTensors[i].GetTensorMutableData<float>();
			auto tensorInfo = outputTensors[i].GetTensorTypeAndShapeInfo();
			size_t elementCount = tensorInfo.GetElementCount();
			outputs.insert(outputs.end(), outputData, outputData + elementCount);
		}

		LOG(OnnxEngine, Info) << "[ONNX] runInference: done";
		return 0;
	} catch (const std::exception &e) {
		LOG(OnnxEngine, Error) << "[ONNX] runInference: exception: " << e.what();
		return -EINVAL;
	}
}

} /* namespace ipa::soft */
} /* namespace libcamera */