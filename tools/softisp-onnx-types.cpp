#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wc++98-compat-extra-semi"
#include <onnxruntime_cxx_api.h>
#pragma GCC diagnostic pop

std::string getTypeName(ONNXTensorElementDataType type) {
    switch(type) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: return "float32";
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16: return "int16";
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32: return "int32";
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64: return "int64";
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8: return "uint8";
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16: return "uint16";
        default: return "unknown";
    }
}

int main() {
    const char* modelDir = std::getenv("SOFTISP_MODEL_DIR");
    if (!modelDir) { std::cerr << "SOFTISP_MODEL_DIR not set" << std::endl; return 1; }
    
    Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "Types");
    Ort::SessionOptions opts;
    Ort::Session session(env, (std::string(modelDir) + "/algo.onnx").c_str(), opts);
    Ort::AllocatorWithDefaultOptions allocator;
    
    std::cout << "algo.onnx input types:" << std::endl;
    for (size_t i = 0; i < session.GetInputCount(); ++i) {
        auto name = session.GetInputNameAllocated(i, allocator);
        auto typeInfo = session.GetInputTypeInfo(i);
        auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
        auto elemType = tensorInfo.GetElementType();
        
        std::cout << "  [" << i << "] " << name.get() << " -> " << getTypeName(elemType) << std::endl;
    }
    
    return 0;
}
