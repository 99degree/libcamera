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
    
    Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "Run");
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
    
    std::cout << "algo.onnx done: " << algoOutputs.size() << " outputs" << std::endl;
    
    /* Load applier.onnx */
    Ort::SessionOptions applierOptions;
    Ort::Session applierSession(env, (std::string(modelDir) + "/applier.onnx").c_str(), applierOptions);
    
    /* Prepare applier inputs - use DIRECT tensor data from algo outputs */
    std::vector<const char*> applierInputNames = {
        "image_desc.input.image.function",
        "image_desc.input.width.function",
        "image_desc.input.frame_id.function",
        "blacklevel.offset.function",
        "awb.wb_gains.function",
        "ccm.ccm.function",
        "tonemap.tonemap_curve.function",
        "gamma.gamma_value.function",
        "yuv.rgb2yuv_matrix.function",
        "chroma.subsample_scale.function"
    };
    
    std::vector<Ort::Value> applierInputs;
    
    std::vector<int16_t> imageCopy(imageData);
    std::vector<int64_t> widthCopy(widthData);
    std::vector<int64_t> frameIdCopy(frameIdData);
    std::vector<float> blackLevelCopy(blackLevelData);
    
    applierInputs.push_back(Ort::Value::CreateTensor(
        allocator, static_cast<void*>(imageCopy.data()), imageCopy.size() * sizeof(int16_t),
        imgShape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16));
    applierInputs.push_back(Ort::Value::CreateTensor(
        allocator, static_cast<void*>(widthCopy.data()), widthCopy.size() * sizeof(int64_t),
        widthShape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64));
    applierInputs.push_back(Ort::Value::CreateTensor(
        allocator, static_cast<void*>(frameIdCopy.data()), frameIdCopy.size() * sizeof(int64_t),
        frameIdShape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64));
    applierInputs.push_back(Ort::Value::CreateTensor(
        allocator, static_cast<void*>(blackLevelCopy.data()), blackLevelCopy.size() * sizeof(float),
        blackLevelShape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT));
    
    /* Add 6 coefficients - check each one */
    int64_t coeffIndices[] = {2, 4, 5, 6, 12, 14};
    const char* coeffNames[] = {
        "awb.wb_gains", "ccm.ccm", "tonemap.tonemap_curve",
        "gamma.gamma_value", "yuv.rgb2yuv_matrix", "chroma.subsample_scale"
    };
    
    std::cout << "Adding coefficients:" << std::endl;
    for (int i = 0; i < 6; ++i) {
        size_t outputIdx = coeffIndices[i];
        auto typeInfo = algoOutputs[outputIdx].GetTensorTypeAndShapeInfo();
        auto shape = typeInfo.GetTensorTypeAndShapeInfo().GetShape();
        auto elemType = typeInfo.GetTensorTypeAndShapeInfo().GetElementType();
        
        std::cout << "  " << coeffNames[i] << ": idx=" << outputIdx 
                  << " type=" << elemType << " shape=[";
        for (size_t j = 0; j < shape.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << shape[j];
        }
        std::cout << "]" << std::endl;
        
        /* Create tensor with correct type */
        if (elemType == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) {
            float* tensorData = algoOutputs[outputIdx].GetTensorMutableData<float>();
            size_t tensorSize = 1;
            for (auto dim : shape) tensorSize *= dim;
            
            std::vector<float> coeffData(tensorData, tensorData + tensorSize);
            std::vector<int64_t> shapeVec(shape.begin(), shape.end());
            
            applierInputs.push_back(Ort::Value::CreateTensor(
                allocator, static_cast<void*>(coeffData.data()), coeffData.size() * sizeof(float),
                shapeVec.data(), shapeVec.size(), ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT));
        } else {
            std::cerr << "  ERROR: Expected float but got type " << elemType << std::endl;
            return 1;
        }
    }
    
    std::cout << "Created " << applierInputs.size() << " applier inputs" << std::endl;
    
    /* Get output names */
    std::vector<std::unique_ptr<char>> applierOutputNamePtrs;
    std::vector<const char*> applierOutputNames;
    for (size_t i = 0; i < applierSession.GetOutputCount(); ++i) {
        auto name = applierSession.GetOutputNameAllocated(i, allocator);
        applierOutputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
        applierOutputNames.push_back(applierOutputNamePtrs.back().get());
    }
    
    /* Run with try-catch */
    std::cout << "Running applier.onnx..." << std::endl;
    try {
        auto applierOutputs = applierSession.Run(
            Ort::RunOptions{nullptr},
            applierInputNames.data(), applierInputs.data(), applierInputNames.size(),
            applierOutputNames.data(), applierOutputNames.size());
        
        std::cout << "✓ applier.onnx succeeded: " << applierOutputs.size() << " outputs" << std::endl;
        return 0;
    } catch (const Ort::Exception& e) {
        std::cerr << "✗ applier.onnx failed: " << e.what() << std::endl;
        return 1;
    }
}
