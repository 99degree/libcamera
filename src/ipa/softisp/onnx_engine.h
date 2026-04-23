/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * OnnxEngine - Wrapper around ONNX Runtime for model inference.
 */
#pragma once

// Suppress warnings from ONNX Runtime headers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wc++98-compat-extra-semi"
#include <onnxruntime_cxx_api.h>
#pragma GCC diagnostic pop

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <libcamera/base/shared_fd.h>
#include <libcamera/geometry.h>

namespace libcamera {
namespace ipa {
namespace soft {

/**
 * OnnxEngine - ONNX Runtime wrapper
 * Manages ONNX sessions and performs inference.
 */
class OnnxEngine {
public:
	OnnxEngine();
	~OnnxEngine();

	int loadModel(const std::string &modelPath);
	int runInference(const std::vector<float> &inputs, std::vector<float> &outputs);
	
	const std::vector<const char*> & getInputNames() const { return inputNames_; }
	const std::vector<const char*> & getOutputNames() const { return outputNames_; }
	bool isLoaded() const { return session_ != nullptr; }
	
	struct TensorInfo {
		std::vector<int64_t> shape;
		ONNXTensorElementDataType type;
		size_t elementCount;
	};
	
	const std::unordered_map<std::string, TensorInfo> & getInputInfo() const { return inputInfo_; }
	const std::unordered_map<std::string, TensorInfo> & getOutputInfo() const { return outputInfo_; }

private:
	Ort::Env env_;
	Ort::SessionOptions sessionOptions_;
	Ort::Session *session_ = nullptr;
	
	std::vector<const char*> inputNames_;
	std::vector<const char*> outputNames_;
	std::unordered_map<std::string, TensorInfo> inputInfo_;
	std::unordered_map<std::string, TensorInfo> outputInfo_;
	
	Ort::MemoryInfo memoryInfo_;
	Ort::AllocatorWithDefaultOptions allocator_;
};

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */
