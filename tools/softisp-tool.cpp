/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * softisp-tool.cpp - Unified SoftISP ONNX Tool Suite
 */

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <sstream>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wc++98-compat-extra-semi"
#include <onnxruntime_cxx_api.h>
#pragma GCC diagnostic pop

std::string getTypeName(ONNXTensorElementDataType type) {
	switch (type) {
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: return "float32";
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16: return "float16";
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_DOUBLE: return "float64";
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32: return "int32";
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64: return "int64";
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8: return "uint8";
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8: return "int8";
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL: return "bool";
		default: return "unknown";
	}
}

void printUsage(const char* prog) {
	std::cout << "Usage: " << prog << " <command> [options]\n\n"
		"Commands:\n"
		"  inspect <model.onnx>          Display model information\n"
		"  shapes <model.onnx>           Check tensor shapes\n"
		"  types <model.onnx>            Display tensor types\n"
		"  all-info <model.onnx>         Comprehensive model info\n"
		"  test <model.onnx>             Basic ONNX test\n"
		"  inference <model.onnx>        Run inference with dummy data\n"
		"  check-coeffs <model.onnx>     Check coefficient tensors\n"
		"  applier-run <model.onnx>      Run coefficient applier\n"
		"  applier-debug <model.onnx>    Debug coefficient applier\n"
		"  save <buffer.raw>             Save raw buffer\n";
}

void printModelInfo(const std::string& path) {
	std::cout << "\n=== Model: " << path << " ===\n";
	try {
		Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "SoftISPTool");
		Ort::SessionOptions opts;
		Ort::Session session(env, path.c_str(), opts);
		Ort::AllocatorWithDefaultOptions allocator;

		std::cout << "\nInputs (" << session.GetInputCount() << "):\n";
		for (size_t i = 0; i < session.GetInputCount(); ++i) {
			auto tensorName = session.GetInputNameAllocated(i, allocator);
			auto inputTypeInfo = session.GetInputTypeInfo(i);
			auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
			auto shape = inputTensorInfo.GetShape();
			auto elemType = inputTensorInfo.GetElementType();
			
			std::cout << "  [" << i << "] " << tensorName.get() 
			          << " | " << getTypeName(elemType)
			          << " | [";
			for (size_t j = 0; j < shape.size(); ++j) {
				if (j > 0) std::cout << ", ";
				std::cout << shape[j];
			}
			std::cout << "]\n";
		}

		std::cout << "\nOutputs (" << session.GetOutputCount() << "):\n";
		for (size_t i = 0; i < session.GetOutputCount(); ++i) {
			auto tensorName = session.GetOutputNameAllocated(i, allocator);
			auto outputTypeInfo = session.GetOutputTypeInfo(i);
			auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
			auto shape = outputTensorInfo.GetShape();
			auto elemType = outputTensorInfo.GetElementType();
			
			std::cout << "  [" << i << "] " << tensorName.get() 
			          << " | " << getTypeName(elemType)
			          << " | [";
			for (size_t j = 0; j < shape.size(); ++j) {
				if (j > 0) std::cout << ", ";
				std::cout << shape[j];
			}
			std::cout << "]\n";
		}
	} catch (const Ort::Exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		exit(1);
	}
}

void checkShapes(const std::string& path) {
	std::cout << "\n=== Shape Check: " << path << " ===\n";
	try {
		Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "SoftISPTool");
		Ort::SessionOptions opts;
		Ort::Session session(env, path.c_str(), opts);
		Ort::AllocatorWithDefaultOptions allocator;

		for (size_t i = 0; i < session.GetInputCount(); ++i) {
			auto tensorName = session.GetInputNameAllocated(i, allocator);
			auto inputTypeInfo = session.GetInputTypeInfo(i);
			auto inputTensorInfo = inputTypeInfo.GetTensorTypeAndShapeInfo();
			auto shape = inputTensorInfo.GetShape();
			
			std::cout << "Input " << i << " (" << tensorName.get() << "): [";
			for (size_t j = 0; j < shape.size(); ++j) {
				if (j > 0) std::cout << " x ";
				std::cout << shape[j];
			}
			std::cout << "]\n";
		}

		for (size_t i = 0; i < session.GetOutputCount(); ++i) {
			auto tensorName = session.GetOutputNameAllocated(i, allocator);
			auto outputTypeInfo = session.GetOutputTypeInfo(i);
			auto outputTensorInfo = outputTypeInfo.GetTensorTypeAndShapeInfo();
			auto shape = outputTensorInfo.GetShape();
			
			std::cout << "Output " << i << " (" << tensorName.get() << "): [";
			for (size_t j = 0; j < shape.size(); ++j) {
				if (j > 0) std::cout << " x ";
				std::cout << shape[j];
			}
			std::cout << "]\n";
		}
	} catch (const Ort::Exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		exit(1);
	}
}

void runTest(const std::string& path) {
	std::cout << "\n=== Test: " << path << " ===\n";
	try {
		Ort::Env env(ORT_LOGGING_LEVEL_ERROR, "SoftISPTool");
		Ort::SessionOptions opts;
		Ort::Session session(env, path.c_str(), opts);
		std::cout << "Model loaded successfully!\n";
		std::cout << "Inputs: " << session.GetInputCount() << "\n";
		std::cout << "Outputs: " << session.GetOutputCount() << "\n";
	} catch (const Ort::Exception& e) {
		std::cerr << "Error: " << e.what() << "\n";
		exit(1);
	}
}

int main(int argc, char* argv[]) {
	if (argc < 2) {
		printUsage(argv[0]);
		return 1;
	}

	std::string command = argv[1];

	if (command == "inspect" || command == "shapes" || command == "all-info" || command == "types") {
		if (argc < 3) {
			std::cerr << "Error: Missing model path\n";
			printUsage(argv[0]);
			return 1;
		}
		printModelInfo(argv[2]);
	} else if (command == "test") {
		if (argc < 3) {
			std::cerr << "Error: Missing model path\n";
			printUsage(argv[0]);
			return 1;
		}
		runTest(argv[2]);
	} else if (command == "inference") {
		std::cout << "Inference test not yet implemented\n";
		return 1;
	} else if (command == "check-coeffs" || command == "applier-run" || command == "applier-debug") {
		std::cout << command << " not yet implemented\n";
		return 1;
	} else if (command == "save") {
		std::cout << "Save not yet implemented\n";
		return 1;
	} else if (command == "help" || command == "-h" || command == "--help") {
		printUsage(argv[0]);
		return 0;
	} else {
		std::cerr << "Unknown command: " << command << "\n";
		printUsage(argv[0]);
		return 1;
	}

	return 0;
}
