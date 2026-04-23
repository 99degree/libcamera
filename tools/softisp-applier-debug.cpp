#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <memory>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wc++98-compat-extra-semi"
#include <onnxruntime_cxx_api.h>
#pragma GCC diagnostic pop

int main() {
    const char* modelDir = std::getenv("SOFTISP_MODEL_DIR");
    if (!modelDir) { std::cerr << "SOFTISP_MODEL_DIR not set" << std::endl; return 1; }
    
    Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "Debug");
    Ort::AllocatorWithDefaultOptions allocator;
    
    /* Load algo.onnx and run */
    Ort::SessionOptions algoOptions;
    Ort::Session algoSession(env, (std::string(modelDir) + "/algo.onnx").c_str(), algoOptions);
    
    std::vector<int16_t> imageData(640*480, 128);
    std::vector<int64_t> widthData(1, 640);
    std::vector<int64_t> frameIdData(1, 1);
    std::vector<float> blackLevelData(1, 0.0f);
    
    int64_t imgShape[] = {480, 640};
    int64_t widthShape[] = {1};
    int64_t frameIdShape[] = {1};
    int64_t blackLevelShape[] = {1};
    
    std::vector<Ort::Value> algoInputs;
    algoInputs.push_back(Ort::Value::CreateTensor(
        allocator, static_cast<void*>(imageData.data()), imageData.size() * sizeof(int16_t),
        imgShape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16));
    algoInputs.push_back(Ort::Value::CreateTensor(
        allocator, static_cast<void*>(widthData.data()), widthData.size() * sizeof(int64_t),
        widthShape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64));
    algoInputs.push_back(Ort::Value::CreateTensor(
        allocator, static_cast<void*>(frameIdData.data()), frameIdData.size() * sizeof(int64_t),
        frameIdShape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64));
    algoInputs.push_back(Ort::Value::CreateTensor(
        allocator, static_cast<void*>(blackLevelData.data()), blackLevelData.size() * sizeof(float),
        blackLevelShape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT));
    
    std::vector<const char*> algoInputNames = {
        "image_desc.input.image.function",
        "image_desc.input.width.function",
        "image_desc.input.frame_id.function",
        "blacklevel.offset.function"
    };
    
    std::vector<std::unique_ptr<char>> algoOutputNamePtrs;
    std::vector<const char*> algoOutputNames;
    for (size_t i = 0; i < algoSession.GetOutputCount(); ++i) {
        auto name = algoSession.GetOutputNameAllocated(i, allocator);
        algoOutputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
        algoOutputNames.push_back(algoOutputNamePtrs.back().get());
    }
    
    std::cout << "Running algo.onnx..." << std::endl;
    auto algoOutputs = algoSession.Run(
        Ort::RunOptions{nullptr},
        algoInputNames.data(), algoInputs.data(), algoInputNames.size(),
        algoOutputNames.data(), algoOutputNames.size());
    
    std::cout << "algo.onnx outputs: " << algoOutputs.size() << std::endl;
    
    /* Check output types */
    for (size_t i = 0; i < std::min(size_t(6), algoOutputs.size()); ++i) {
        auto typeInfo = algoOutputs[i].GetTensorTypeAndShapeInfo();
        auto elemType = typeInfo.GetElementType();
        auto shape = typeInfo.GetShape();
        
        std::cout << "  Output " << i << ": type=" << elemType << " shape=[";
        for (size_t j = 0; j < shape.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << shape[j];
        }
        std::cout << "]" << std::endl;
    }
    
    /* Load applier.onnx */
    Ort::SessionOptions applierOptions;
    Ort::Session applierSession(env, (std::string(modelDir) + "/applier.onnx").c_str(), applierOptions);
    
    std::cout << "\napplier.onnx inputs:" << std::endl;
    for (size_t i = 0; i < applierSession.GetInputCount(); ++i) {
        auto name = applierSession.GetInputNameAllocated(i, allocator);
        auto typeInfo = applierSession.GetInputTypeInfo(i);
        auto elemType = typeInfo.GetTensorTypeAndShapeInfo().GetElementType();
        auto shape = typeInfo.GetTensorTypeAndShapeInfo().GetShape();
        
        std::cout << "  [" << i << "] " << name.get() << " type=" << elemType << " shape=[";
        for (size_t j = 0; j < shape.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << shape[j];
        }
        std::cout << "]" << std::endl;
    }
    
    return 0;
}
