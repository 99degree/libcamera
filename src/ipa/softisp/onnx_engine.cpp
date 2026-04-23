/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * onnx_engine.cpp - ONNX Runtime Engine for SoftISP
 */

#include "onnx_engine.h"

#include <cstring>

namespace libcamera {
namespace ipa {
namespace softisp {

OnnxEngine::OnnxEngine()
    : env_(ORT_LOGGING_LEVEL_ERROR, "SoftISP"),
      sessionOptions_()
{
    sessionOptions_.SetIntraOpNumThreads(2);
    sessionOptions_.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
}

OnnxEngine::~OnnxEngine()
{
    session_.reset();
}

bool OnnxEngine::loadModel(const std::string &modelPath)
{
    modelName_ = modelPath.substr(modelPath.find_last_of("/\\") + 1);
    
    try {
        session_ = std::make_unique<Ort::Session>(env_, modelPath.c_str(), sessionOptions_);
        
        size_t inputCount = session_->GetInputCount();
        inputInfo_.clear();
        inputNames_.clear();
        
        for (size_t i = 0; i < inputCount; ++i) {
            auto name = session_->GetInputNameAllocated(i, allocator_);
            inputNames_.push_back(name.get());
            
            auto typeInfo = session_->GetInputTypeInfo(i);
            auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
            auto shape = tensorInfo.GetShape();
            auto elemType = tensorInfo.GetElementType();
            
            inputInfo_.emplace_back(name.get(), shape, elemType);
        }
        
        size_t outputCount = session_->GetOutputCount();
        outputInfo_.clear();
        outputNames_.clear();
        
        for (size_t i = 0; i < outputCount; ++i) {
            auto name = session_->GetOutputNameAllocated(i, allocator_);
            outputNames_.push_back(name.get());
            
            auto typeInfo = session_->GetOutputTypeInfo(i);
            auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
            auto shape = tensorInfo.GetShape();
            auto elemType = tensorInfo.GetElementType();
            
            outputInfo_.emplace_back(name.get(), shape, elemType);
        }
        
        return true;
        
    } catch (const Ort::Exception &e) {
        return false;
    }
}

bool OnnxEngine::runInference(
    const std::unordered_map<std::string, std::vector<float>> &inputs,
    std::unordered_map<std::string, std::vector<float>> &outputs)
{
    if (!session_) {
        return false;
    }
    
    try {
        std::vector<Ort::Value> inputTensors;
        inputTensors.reserve(inputNames_.size());
        
        for (const auto &name : inputNames_) {
            auto it = inputs.find(name);
            if (it == inputs.end()) {
                return false;
            }
            
            std::vector<int64_t> shape;
            for (const auto &info : inputInfo_) {
                if (std::get<0>(info) == name) {
                    shape = std::get<1>(info);
                    break;
                }
            }
            
            auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            auto tensor = Ort::Value::CreateTensor<float>(
                memoryInfo,
                const_cast<float*>(it->second.data()),
                it->second.size(),
                shape.data(),
                shape.size());
            
            inputTensors.push_back(std::move(tensor));
        }
        
        auto outputTensors = session_->Run(
            Ort::RunOptions{nullptr},
            inputNames_.data(),
            inputTensors.data(),
            inputTensors.size(),
            outputNames_.data(),
            outputNames_.size());
        
        outputs.clear();
        for (size_t i = 0; i < outputNames_.size(); ++i) {
            auto tensorInfo = outputTensors[i].GetTensorTypeAndShapeInfo();
            auto shape = tensorInfo.GetShape();
            auto elemType = tensorInfo.GetElementType();
            
            if (elemType == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                const float *data = outputTensors[i].GetTensorData<float>();
                size_t size = 1;
                for (auto dim : shape) size *= dim;
                
                std::vector<float> outputData(data, data + size);
                outputs[outputNames_[i]] = std::move(outputData);
            }
        }
        
        return true;
        
    } catch (const Ort::Exception &e) {
        return false;
    }
}

bool OnnxEngine::runInference(
    const std::unordered_map<std::string, std::vector<int64_t>> &inputs,
    std::unordered_map<std::string, std::vector<float>> &outputs)
{
    if (!session_) {
        return false;
    }
    
    try {
        std::vector<Ort::Value> inputTensors;
        inputTensors.reserve(inputNames_.size());
        
        for (const auto &name : inputNames_) {
            auto it = inputs.find(name);
            if (it == inputs.end()) {
                return false;
            }
            
            std::vector<int64_t> shape;
            for (const auto &info : inputInfo_) {
                if (std::get<0>(info) == name) {
                    shape = std::get<1>(info);
                    break;
                }
            }
            
            auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            auto tensor = Ort::Value::CreateTensor<int64_t>(
                memoryInfo,
                const_cast<int64_t*>(it->second.data()),
                it->second.size(),
                shape.data(),
                shape.size());
            
            inputTensors.push_back(std::move(tensor));
        }
        
        auto outputTensors = session_->Run(
            Ort::RunOptions{nullptr},
            inputNames_.data(),
            inputTensors.data(),
            inputTensors.size(),
            outputNames_.data(),
            outputNames_.size());
        
        outputs.clear();
        for (size_t i = 0; i < outputNames_.size(); ++i) {
            auto tensorInfo = outputTensors[i].GetTensorTypeAndShapeInfo();
            auto shape = tensorInfo.GetShape();
            auto elemType = tensorInfo.GetElementType();
            
            if (elemType == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
                const float *data = outputTensors[i].GetTensorData<float>();
                size_t size = 1;
                for (auto dim : shape) size *= dim;
                
                std::vector<float> outputData(data, data + size);
                outputs[outputNames_[i]] = std::move(outputData);
            }
        }
        
        return true;
        
    } catch (const Ort::Exception &e) {
        return false;
    }
}

} // namespace softisp
} // namespace ipa
} // namespace libcamera
