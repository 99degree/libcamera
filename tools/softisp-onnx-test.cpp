/*
 * softisp-onnx-test.cpp - ONNX Model Information Display
 */

#include <iostream>
#include <string>
#include <cstdlib>
#include <onnxruntime_cxx_api.h>

int testModel(const std::string& modelPath, const std::string& modelName) {
    std::cout << "\n=== " << modelName << " ===" << std::endl;
    std::cout << "Path: " << modelPath << std::endl;
    
    try {
        Ort::Env env(ORT_LOGGING_LEVEL_ERROR, modelName.c_str());
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(1);
        
        std::cout << "Loading..." << std::endl;
        Ort::Session session(env, modelPath.c_str(), sessionOptions);
        std::cout << "✓ Loaded" << std::endl;
        
        auto inputCount = session.GetInputCount();
        auto outputCount = session.GetOutputCount();
        
        std::cout << "Inputs: " << inputCount << ", Outputs: " << outputCount << std::endl;
        
        std::cout << "Input names:" << std::endl;
        Ort::AllocatorWithDefaultOptions allocator;
        for (size_t i = 0; i < inputCount; ++i) {
            auto name = session.GetInputNameAllocated(i, allocator);
            std::cout << "  - " << name.get() << std::endl;
        }
        
        std::cout << "Output names:" << std::endl;
        for (size_t i = 0; i < outputCount; ++i) {
            auto name = session.GetOutputNameAllocated(i, allocator);
            std::cout << "  - " << name.get() << std::endl;
        }
        
        return 0;
    } catch (const Ort::Exception& e) {
        std::cerr << "✗ Error: " << e.what() << std::endl;
        return 1;
    }
}

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  SoftISP ONNX Model Inspector" << std::endl;
    std::cout << "========================================" << std::endl;
    
    const char* modelDir = std::getenv("SOFTISP_MODEL_DIR");
    if (!modelDir) {
        std::cerr << "Error: SOFTISP_MODEL_DIR not set" << std::endl;
        return 1;
    }
    
    std::cout << "Model directory: " << modelDir << std::endl;
    
    int ret = 0;
    ret |= testModel(std::string(modelDir) + "/algo.onnx", "algo.onnx");
    ret |= testModel(std::string(modelDir) + "/applier.onnx", "applier.onnx");
    
    std::cout << "\n========================================" << std::endl;
    std::cout << (ret == 0 ? "All models valid ✓" : "Errors ✗") << std::endl;
    std::cout << "========================================" << std::endl;
    
    return ret;
}
