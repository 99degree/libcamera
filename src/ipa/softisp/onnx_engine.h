/*
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * onnx_engine.h - ONNX Runtime Engine for SoftISP
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

#include <onnxruntime_cxx_api.h>

namespace libcamera {
namespace ipa {
namespace softisp {

/**
 * @class OnnxEngine
 * @brief Wrapper around ONNX Runtime for model inference
 *
 * Handles model loading, session management, and inference execution.
 */
class OnnxEngine {
public:
	OnnxEngine();
	~OnnxEngine();

	/**
	 * @brief Load an ONNX model from file
	 * @param modelPath Path to the .onnx file
	 * @return true if loaded successfully, false otherwise
	 */
	bool loadModel(const std::string &modelPath);

	/**
	 * @brief Run inference with given inputs
	 * @param inputs Map of input tensor names to data
	 * @param outputs Map to store output tensor data
	 * @return true if inference succeeded, false otherwise
	 */
	bool runInference(
		const std::unordered_map<std::string, std::vector<float>> &inputs,
		std::unordered_map<std::string, std::vector<float>> &outputs);

	/**
	 * @brief Run inference with integer inputs
	 * @param inputs Map of input tensor names to int64 data
	 * @param outputs Map to store output tensor data
	 * @return true if inference succeeded, false otherwise
	 */
	bool runInference(
		const std::unordered_map<std::string, std::vector<int64_t>> &inputs,
		std::unordered_map<std::string, std::vector<float>> &outputs);

	/**
	 * @brief Get input tensor information
	 * @return Vector of tuples (name, shape, type)
	 */
	const std::vector<std::tuple<std::string, std::vector<int64_t>, ONNXTensorElementDataType>> &inputInfo() const { return inputInfo_; }

	/**
	 * @brief Get output tensor information
	 * @return Vector of tuples (name, shape, type)
	 */
	const std::vector<std::tuple<std::string, std::vector<int64_t>, ONNXTensorElementDataType>> &outputInfo() const { return outputInfo_; }

	/**
	 * @brief Check if model is loaded
	 */
	bool isLoaded() const { return session_ != nullptr; }

	/**
	 * @brief Get model name
	 */
	const std::string &modelName() const { return modelName_; }

private:
	std::string modelName_;
	std::unique_ptr<Ort::Session> session_;
	Ort::Env env_;
	Ort::SessionOptions sessionOptions_;

	std::vector<std::tuple<std::string, std::vector<int64_t>, ONNXTensorElementDataType>> inputInfo_;
	std::vector<std::tuple<std::string, std::vector<int64_t>, ONNXTensorElementDataType>> outputInfo_;

	std::vector<const char *> inputNames_;
	std::vector<const char *> outputNames_;

	// Memory allocator
	Ort::AllocatorWithDefaultOptions allocator_;
};

} // namespace softisp
} // namespace ipa
} // namespace libcamera
