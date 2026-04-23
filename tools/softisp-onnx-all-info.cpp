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
    
    Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "Info");
    Ort::SessionOptions opts;
    Ort::Session session(env, (std::string(modelDir) + "/algo.onnx").c_str(), opts);
    Ort::AllocatorWithDefaultOptions allocator;
    
    std::cout << "=== algo.onnx ===" << std::endl;
    std::cout << "Inputs (" << session.GetInputCount() << "):" << std::endl;
    for (size_t i = 0; i < session.GetInputCount(); ++i) {
        auto name = session.GetInputNameAllocated(i, allocator);
        std::cout << "  [" << i << "] '" << name.get() << "'" << std::endl;
    }
    
    std::cout << "\nOutputs (" << session.GetOutputCount() << "):" << std::endl;
    for (size_t i = 0; i < session.GetOutputCount(); ++i) {
        auto name = session.GetOutputNameAllocated(i, allocator);
        std::cout << "  [" << i << "] '" << name.get() << "'" << std::endl;
    }
    
    return 0;
}
