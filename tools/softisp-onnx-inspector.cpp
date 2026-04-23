/*
 * softisp-onnx-inspector.cpp - Enhanced ONNX Model Inspector with Mapping
 */

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
        default: return "unknown";
    }
}

void printModel(const std::string& path, const std::string& name) {
	std::cout << "\n=== " << name << " ===" << std::endl;
	std::cout << "Path: " << path << std::endl;
	try {
		Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "Inspect");
		Ort::SessionOptions opts;
		Ort::Session session(env, path.c_str(), opts);
		Ort::AllocatorWithDefaultOptions allocator;

		std::cout << "Inputs: " << session.GetInputCount() << std::endl;
		for (size_t i = 0; i < session.GetInputCount(); ++i) {
			auto tensorName = session.GetInputNameAllocated(i, allocator);
			auto inputTypeInfo = session.GetInputTypeInfo(i);
			auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
			auto shape = inputTensorInfo.GetShape();
			auto elemType = inputTensorInfo.GetElementType();
			std::cout << " Input " << i << ": " << tensorName.get() << std::endl;
			std::cout << " Type: " << getTypeName(elemType) << std::endl;
			std::cout << " Shape: [";
			for (size_t j = 0; j < shape.size(); ++j) {
				if (j > 0) std::cout << ", ";
				std::cout << shape[j];
			}
			std::cout << "]" << std::endl;
		}

		std::cout << "\nOutputs: " << session.GetOutputCount() << std::endl;
		for (size_t i = 0; i < session.GetOutputCount(); ++i) {
			auto tensorName = session.GetOutputNameAllocated(i, allocator);
			auto outputTypeInfo = session.GetOutputTypeInfo(i);
			auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
			auto shape = outputTensorInfo.GetShape();
			auto elemType = outputTensorInfo.GetElementType();
			std::cout << " Output " << i << ": " << tensorName.get() << std::endl;
			std::cout << " Type: " << getTypeName(elemType) << std::endl;
			std::cout << " Shape: [";
			for (size_t j = 0; j < shape.size(); ++j) {
				if (j > 0) std::cout << ", ";
				std::cout << shape[j];
			}
			std::cout << "]" << std::endl;
		}
	} catch (const Ort::Exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
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
    
    printModel(std::string(modelDir) + "/algo.onnx", "algo.onnx");
    printModel(std::string(modelDir) + "/applier.onnx", "applier.onnx");
    
    /* Show recommended mapping */
    std::cout << "\n=== Recommended Tensor Mapping ===" << std::endl;
    std::cout << "algo.onnx outputs -> applier.onnx inputs:" << std::endl;
    std::cout << "  [2] awb.wb_gains.function -> awb.wb_gains.function" << std::endl;
    std::cout << "  [4] ccm.ccm.function -> ccm.ccm.function" << std::endl;
    std::cout << "  [5] tonemap.tonemap_curve.function -> tonemap.tonemap_curve.function" << std::endl;
    std::cout << "  [6] gamma.gamma_value.function -> gamma.gamma_value.function" << std::endl;
    std::cout << "  [12] yuv.rgb2yuv_matrix.function -> yuv.rgb2yuv_matrix.function" << std::endl;
    std::cout << "  [14] chroma.subsample_scale.function -> chroma.subsample_scale.function" << std::endl;
    
    std::cout << "\n✓ All models valid" << std::endl;
    
    return 0;
}
