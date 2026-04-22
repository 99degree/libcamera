/*
 * softisp-onnx-inference-test.cpp - Full ONNX Inference Test
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <memory>
#include <onnxruntime_cxx_api.h>

int runFullInference(const std::string& modelDir) {
    std::cout << "\n=== Running Full Inference Pipeline ===" << std::endl;
    
    Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "SoftISP");
    Ort::AllocatorWithDefaultOptions allocator;
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    
    /* Load algo.onnx */
    std::string algoPath = modelDir + "/algo.onnx";
    std::cout << "Loading algo.onnx..." << std::endl;
    Ort::SessionOptions algoOptions;
    algoOptions.SetIntraOpNumThreads(1);
    Ort::Session algoSession(env, algoPath.c_str(), algoOptions);
    std::cout << "✓ algo.onnx loaded" << std::endl;
    
    /* Prepare inputs */
    int imgWidth = 640, imgHeight = 480;
    size_t imgSize = imgWidth * imgHeight;
    std::vector<int16_t> imageData(imgSize, 128);
    int64_t imgShape[] = {imgHeight, imgWidth};
    
    std::vector<int64_t> widthData(1, imgWidth);
    int64_t widthShape[] = {1};
    
    std::vector<int64_t> frameIdData(1, 1);
    int64_t frameIdShape[] = {1};
    
    std::vector<float> blackLevelData(1, 0.0f);
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
    
    /* Get input names */
    std::vector<std::unique_ptr<char>> algoInputNamePtrs;
    std::vector<const char*> algoInputNames;
    for (size_t i = 0; i < 4; ++i) {
        auto name = algoSession.GetInputNameAllocated(i, allocator);
        algoInputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
        algoInputNames.push_back(algoInputNamePtrs.back().get());
    }
    
    std::cout << "  Created 4 input tensors" << std::endl;
    
    /* Get output names for algo.onnx */
    std::vector<std::unique_ptr<char>> algoOutputNamePtrs;
    std::vector<const char*> algoOutputNames;
    for (size_t i = 0; i < algoSession.GetOutputCount(); ++i) {
        auto name = algoSession.GetOutputNameAllocated(i, allocator);
        algoOutputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
        algoOutputNames.push_back(algoOutputNamePtrs.back().get());
    }
    
    /* Run algo.onnx */
    std::cout << "\nRunning algo.onnx inference..." << std::endl;
    auto algoOutputs = algoSession.Run(
        Ort::RunOptions{nullptr},
        algoInputNames.data(), algoInputs.data(), algoInputNames.size(),
        algoOutputNames.data(), algoOutputNames.size());
    
    std::cout << "✓ algo.onnx completed: " << algoOutputs.size() << " outputs" << std::endl;
    
    /* Load applier.onnx */
    std::string applierPath = modelDir + "/applier.onnx";
    std::cout << "\nLoading applier.onnx..." << std::endl;
    Ort::SessionOptions applierOptions;
    applierOptions.SetIntraOpNumThreads(1);
    Ort::Session applierSession(env, applierPath.c_str(), applierOptions);
    std::cout << "✓ applier.onnx loaded" << std::endl;
    
    /* Create IoBinding */
    Ort::IoBinding ioBinding(applierSession);
    
    /* Bind original 4 inputs */
    std::cout << "  Binding 4 original inputs..." << std::endl;
    ioBinding.BindInput(algoInputNames[0], algoInputs[0]);
    ioBinding.BindInput(algoInputNames[1], algoInputs[1]);
    ioBinding.BindInput(algoInputNames[2], algoInputs[2]);
    ioBinding.BindInput(algoInputNames[3], algoInputs[3]);
    
    /* Bind 6 coefficient tensors */
    std::cout << "  Binding 6 coefficient tensors..." << std::endl;
    
    struct Mapping {
        size_t algoOutputIdx;
        const char* applierInputName;
    };
    
    Mapping mappings[] = {
        {2, "awb.wb_gains.function"},
        {4, "ccm.ccm.function"},
        {5, "tonemap.tonemap_curve.function"},
        {6, "gamma.gamma_value.function"},
        {12, "yuv.rgb2yuv_matrix.function"},
        {14, "chroma.subsample_scale.function"}
    };
    
    for (int i = 0; i < 6; ++i) {
        size_t outputIdx = mappings[i].algoOutputIdx;
        const char* inputName = mappings[i].applierInputName;
        
        std::cout << "    [" << i << "] algo[" << outputIdx << "] (" << algoOutputNames[outputIdx] 
                  << ") -> applier \"" << inputName << "\"" << std::endl;
        
        ioBinding.BindInput(inputName, std::move(algoOutputs[outputIdx]));
    }
    
    /* Bind outputs - request all outputs by name */
    std::cout << "  Binding outputs..." << std::endl;
    
    std::vector<std::unique_ptr<char>> applierOutputNamePtrs;
    std::vector<const char*> applierOutputNames;
    for (size_t i = 0; i < applierSession.GetOutputCount(); ++i) {
        auto name = applierSession.GetOutputNameAllocated(i, allocator);
        applierOutputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
        applierOutputNames.push_back(applierOutputNamePtrs.back().get());
        
        /* Bind output to memory info - let ONNX allocate */
        ioBinding.BindOutput(applierOutputNames[i], memoryInfo);
    }
    
    std::cout << "    Bound " << applierOutputNames.size() << " outputs" << std::endl;
    
    /* Run applier.onnx */
    std::cout << "\nRunning applier.onnx inference..." << std::endl;
    try {
        applierSession.Run(Ort::RunOptions{nullptr}, ioBinding);
        
        /* Get outputs */
        auto applierOutputs = ioBinding.GetOutputValues();
        
        std::cout << "✓ applier.onnx completed successfully!" << std::endl;
        std::cout << "  Got " << applierOutputs.size() << " output tensors" << std::endl;
        
        std::cout << "\n=== Full Inference Pipeline SUCCEEDED ===" << std::endl;
        std::cout << "  algo.onnx: 4 inputs -> 15 outputs" << std::endl;
        std::cout << "  applier.onnx: 10 inputs -> " << applierOutputs.size() << " outputs" << std::endl;
        std::cout << "  IoBinding successfully connected models!" << std::endl;
        
        return 0;
        
    } catch (const Ort::Exception& e) {
        std::cerr << "\n✗ Inference failed: " << e.what() << std::endl;
        return 1;
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  SoftISP Full Inference Test" << std::endl;
    std::cout << "========================================" << std::endl;
    
    const char* modelDir = std::getenv("SOFTISP_MODEL_DIR");
    if (!modelDir) {
        std::cerr << "Error: SOFTISP_MODEL_DIR not set" << std::endl;
        return 1;
    }
    
    std::cout << "Model directory: " << modelDir << std::endl;
    
    int ret = runFullInference(modelDir);
    
    std::cout << "\n========================================" << std::endl;
    std::cout << (ret == 0 ? "TEST PASSED ✓" : "TEST FAILED ✗") << std::endl;
    std::cout << "========================================" << std::endl;
    
    return ret;
}
