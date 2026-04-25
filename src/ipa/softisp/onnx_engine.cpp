/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * OnnxEngine - Implementation of ONNX Runtime wrapper.
 */
#include "onnx_engine.h"
#include <libcamera/base/log.h>
#include <algorithm>
#include <unistd.h>

namespace libcamera {
LOG_DEFINE_CATEGORY(OnnxEngine)

namespace ipa::soft {

OnnxEngine::OnnxEngine()
    : env_(ORT_LOGGING_LEVEL_WARNING, "SoftIsp"),
      memoryInfo_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault))
{
    sessionOptions_.SetIntraOpNumThreads(2);
    sessionOptions_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}

OnnxEngine::~OnnxEngine()
{
    if (session_) {
        delete session_;
        session_ = nullptr;
    }
}

int OnnxEngine::loadModel(const std::string &modelPath)
{
    if (!modelPath.empty() && access(modelPath.c_str(), R_OK) != 0) {
        return -ENOENT;
    }

    try {
        session_ = new Ort::Session(env_, modelPath.c_str(), sessionOptions_);

        size_t numInputs = session_->GetInputCount();
        inputNames_.reserve(numInputs);
        inputInfo_.clear();

        Ort::AllocatorWithDefaultOptions allocator;
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
        outputNames_.reserve(numOutputs);
        outputInfo_.clear();

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

        for (const auto &name : inputNames_)
        for (const auto &name : outputNames_)

        return 0;

    } catch (const Ort::Exception &e) {
        if (session_) {
            delete session_;
            session_ = nullptr;
        }
        return -EINVAL;
    }
}

int OnnxEngine::runInference(const std::vector<float> &inputs,
                             std::vector<float> &outputs)
{
    if (!session_) {
        return -ENODEV;
    }

    if (inputs.empty()) {
        return -EINVAL;
    }

    try {
        const auto &info = inputInfo_.begin()->second;
        size_t inputSize = info.elementCount;

        if (inputs.size() != inputSize) {
            return -EINVAL;
        }

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(
            OrtArenaAllocator, OrtMemTypeDefault);

        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, const_cast<float*>(inputs.data()),
            inputSize, info.shape.data(), info.shape.size());

        std::vector<const char*> inputNamesPtr = inputNames_;
        std::vector<const char*> outputNamesPtr = outputNames_;

        auto outputs_ort = session_->Run(
            Ort::RunOptions{nullptr},
            inputNamesPtr.data(), &inputTensor, 1,
            outputNamesPtr.data(), outputNames_.size());

        // Get output data from first output tensor
        size_t outputSize = outputInfo_.begin()->second.elementCount;
        outputs.resize(outputSize);

        const float *outputData = outputs_ort[0].GetTensorData<float>();
        std::copy(outputData, outputData + outputSize, outputs.begin());

        return 0;

    } catch (const Ort::Exception &e) {
        return -EIO;
    }
}

} /* namespace ipa */
} /* namespace libcamera */
