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
    
    Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "Check");
    Ort::SessionOptions algoOptions;
    Ort::Session algoSession(env, (std::string(modelDir) + "/algo.onnx").c_str(), algoOptions);
    Ort::AllocatorWithDefaultOptions allocator;
    
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
    
    auto algoOutputs = algoSession.Run(
        Ort::RunOptions{nullptr},
        algoInputNames.data(), algoInputs.data(), algoInputNames.size(),
        algoOutputNames.data(), algoOutputNames.size());
    
    std::cout << "Checking coefficient outputs:" << std::endl;
    
    /* Check output 14: chroma.subsample_scale */
    std::cout << "\nOutput 14 (chroma.subsample_scale):" << std::endl;
    auto typeInfo = algoOutputs[14].GetTensorTypeAndShapeInfo();
    auto shape = typeInfo.GetShape();
    float* data = algoOutputs[14].GetTensorMutableData<float>();
    size_t size = 1;
    for (auto dim : shape) size *= dim;
    
    std::cout << "  Shape: [";
    for (size_t i = 0; i < shape.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << shape[i];
    }
    std::cout << "]" << std::endl;
    std::cout << "  Values: [";
    for (size_t i = 0; i < size; ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << data[i];
    }
    std::cout << "]" << std::endl;
    
    /* Check all coefficient outputs */
    int64_t coeffIndices[] = {2, 4, 5, 6, 12, 14};
    const char* coeffNames[] = {
        "awb.wb_gains",
        "ccm.ccm",
        "tonemap.tonemap_curve",
        "gamma.gamma_value",
        "yuv.rgb2yuv_matrix",
        "chroma.subsample_scale"
    };
    
    std::cout << "\nAll coefficient outputs:" << std::endl;
    for (int i = 0; i < 6; ++i) {
        size_t idx = coeffIndices[i];
        typeInfo = algoOutputs[idx].GetTensorTypeAndShapeInfo();
        shape = typeInfo.GetShape();
        data = algoOutputs[idx].GetTensorMutableData<float>();
        size = 1;
        for (auto dim : shape) size *= dim;
        
        float minVal = data[0], maxVal = data[0];
        for (size_t j = 1; j < size; ++j) {
            if (data[j] < minVal) minVal = data[j];
            if (data[j] > maxVal) maxVal = data[j];
        }
        
        std::cout << "  " << coeffNames[i] << ": min=" << minVal << " max=" << maxVal << std::endl;
    }
    
    return 0;
}
