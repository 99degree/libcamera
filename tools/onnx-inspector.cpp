/*
 * onnx-inspector.cpp - ONNX Model Inspection Tool
 *
 * A utility to inspect ONNX models used by SoftISP:
 * - Display model metadata (inputs, outputs, shapes)
 * - Run inference with synthetic/test data
 * - Verify model compatibility
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <onnxruntime_cxx_api.h>

void printUsage(const char* prog) {
    std::cerr << "Usage: " << prog << " [options] <model.onnx>" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  -i, --info       Show model information only" << std::endl;
    std::cerr << "  -r, --run        Run inference with test data" << std::endl;
    std::cerr << "  -h, --help       Show this help" << std::endl;
}

void printTensorInfo(const char* name, const Ort::TypeInfo& typeInfo) {
    auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
    auto shape = tensorInfo.GetShape();
    auto elemType = tensorInfo.GetElementType();
    
    std::cout << "  " << name << ":" << std::endl;
    std::cout << "    Shape: [";
    for (size_t i = 0; i < shape.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << shape[i];
    }
    std::cout << "]" << std::endl;
    
    std::string typeName;
    switch (elemType) {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: typeName = "float"; break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16: typeName = "float16"; break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE: typeName = "double"; break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32: typeName = "int32"; break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64: typeName = "int64"; break;
        default: typeName = "unknown"; break;
    }
    std::cout << "    Type: " << typeName << std::endl;
}

int main(int argc, char* argv[]) {
    bool showInfo = true;
    bool runInference = false;
    std::string modelPath;
    
    /* Parse arguments */
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-i" || arg == "--info") {
            showInfo = true;
        } else if (arg == "-r" || arg == "--run") {
            runInference = true;
        } else if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg[0] != '-') {
            modelPath = arg;
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (modelPath.empty()) {
        std::cerr << "Error: No model file specified" << std::endl;
        printUsage(argv[0]);
        return 1;
    }
    
    std::cout << "=== ONNX Model Inspector ===" << std::endl;
    std::cout << "Model: " << modelPath << std::endl;
    std::cout << std::endl;
    
    /* Initialize ONNX Runtime */
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "OnnxInspector");
    Ort::SessionOptions sessionOptions;
    sessionOptions.SetIntraOpNumThreads(1);
    
    /* Load model */
    std::cout << "Loading model..." << std::endl;
    Ort::Session session(env, modelPath.c_str(), sessionOptions);
    std::cout << "Model loaded successfully" << std::endl;
    std::cout << std::endl;
    
    /* Get input/output information */
    auto inputCount = session.GetInputCount();
    auto outputCount = session.GetOutputCount();
    
    std::cout << "Model Statistics:" << std::endl;
    std::cout << "  Inputs: " << inputCount << std::endl;
    std::cout << "  Outputs: " << outputCount << std::endl;
    std::cout << std::endl;
    
    if (showInfo) {
        /* Print input details */
        std::cout << "Inputs:" << std::endl;
        for (size_t i = 0; i < inputCount; ++i) {
            auto name = session.GetInputNameAllocated(i, Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
            auto typeInfo = session.GetInputTypeInfo(i);
            printTensorInfo(name.get(), typeInfo);
            std::cout << std::endl;
        }
        
        /* Print output details */
        std::cout << "Outputs:" << std::endl;
        for (size_t i = 0; i < outputCount; ++i) {
            auto name = session.GetOutputNameAllocated(i, Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
            auto typeInfo = session.GetOutputTypeInfo(i);
            printTensorInfo(name.get(), typeInfo);
            std::cout << std::endl;
        }
    }
    
    if (runInference) {
        std::cout << "Running inference with test data..." << std::endl;
        
        /* Prepare input tensors */
        std::vector<Ort::Value> inputTensors;
        std::vector<const char*> inputNames;
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        
        for (size_t i = 0; i < inputCount; ++i) {
            auto name = session.GetInputNameAllocated(i, memoryInfo);
            inputNames.push_back(name.get());
            
            auto typeInfo = session.GetInputTypeInfo(i);
            auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
            auto shape = tensorInfo.GetShape();
            
            /* Calculate total elements */
            size_t totalElements = 1;
            for (auto dim : shape) {
                totalElements *= dim;
            }
            
            /* Create test data (fill with 0.5) */
            std::vector<float> testData(totalElements, 0.5f);
            
            /* Create tensor */
            inputTensors.push_back(Ort::Value::CreateTensor<float>(
                memoryInfo, testData.data(), testData.size(), 
                shape.data(), shape.size()));
            
            std::cout << "  Created input tensor for: " << name.get() 
                      << " (size: " << totalElements << ")" << std::endl;
        }
        
        /* Run inference */
        try {
            auto outputTensors = session.Run(
                Ort::RunOptions{nullptr},
                inputNames.data(), inputTensors.data(), inputCount,
                nullptr, 0);  /* Output names - will get all */
            
            std::cout << std::endl;
            std::cout << "Inference completed successfully!" << std::endl;
            std::cout << "Outputs generated: " << outputTensors.size() << std::endl;
            
            /* Print output shapes */
            for (size_t i = 0; i < outputCount; ++i) {
                auto name = session.GetOutputNameAllocated(i, memoryInfo);
                auto shape = outputTensors[i].GetTensorTypeAndShapeInfo().GetShape();
                
                std::cout << "  Output " << i << " (" << name.get() << "): [";
                for (size_t j = 0; j < shape.size(); ++j) {
                    if (j > 0) std::cout << ", ";
                    std::cout << shape[j];
                }
                std::cout << "]" << std::endl;
            }
            
        } catch (const Ort::Exception& e) {
            std::cerr << "Inference failed: " << e.what() << std::endl;
            return 1;
        }
    }
    
    std::cout << std::endl;
    std::cout << "=== Inspection Complete ===" << std::endl;
    
    return 0;
}
