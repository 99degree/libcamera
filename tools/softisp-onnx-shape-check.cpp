#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wc++98-compat-extra-semi"
#include <onnxruntime_cxx_api.h>
#pragma GCC diagnostic pop

int main() {
    const char* modelDir = std::getenv("SOFTISP_MODEL_DIR");
    if (!modelDir) { std::cerr << "SOFTISP_MODEL_DIR not set" << std::endl; return 1; }
    
    Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "Check");
    Ort::SessionOptions opts;
    Ort::Session session(env, (std::string(modelDir) + "/algo.onnx").c_str(), opts);
    Ort::AllocatorWithDefaultOptions allocator;
    
    std::cout << "algo.onnx shapes:" << std::endl;
    for (size_t i = 0; i < session.GetInputCount(); ++i) {
        auto name = session.GetInputNameAllocated(i, allocator);
        auto typeInfo = session.GetInputTypeInfo(i);
        auto shape = typeInfo.GetTensorTypeAndShapeInfo().GetShape();
        
        std::cout << "  Input " << i << " " << name.get() << ": [";
        for (size_t j = 0; j < shape.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << shape[j];
        }
        std::cout << "]" << std::endl;
    }
    
    std::cout << "\nOutputs:" << std::endl;
    for (size_t i = 0; i < session.GetOutputCount(); ++i) {
        auto name = session.GetOutputNameAllocated(i, allocator);
        auto typeInfo = session.GetOutputTypeInfo(i);
        auto shape = typeInfo.GetTensorTypeAndShapeInfo().GetShape();
        
        std::cout << "  Output " << i << " " << name.get() << ": [";
        for (size_t j = 0; j < shape.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << shape[j];
        }
        std::cout << "]" << std::endl;
    }
    
    return 0;
}
