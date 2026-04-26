/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

// ORT headers must be at file scope, not inside namespace
#ifdef USE_ONNX_ORT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wc++98-compat-extra-semi"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <onnxruntime_cxx_api.h>
#pragma GCC diagnostic pop
#endif

namespace libcamera {
namespace ipa {
namespace soft {

struct TensorInfo {
	std::vector<int64_t> shape;
	int type;
	size_t elementCount;
};

class OnnxEngine {
public:
	virtual ~OnnxEngine() = default;

	virtual int loadModel(const std::string &modelPath) = 0;
	virtual int runInference(const std::vector<float> &inputs, std::vector<float> &outputs) = 0;

	virtual const std::vector<const char *> &getInputNames() const = 0;
	virtual const std::vector<const char *> &getOutputNames() const = 0;
	virtual bool isLoaded() const = 0;
	virtual const std::unordered_map<std::string, TensorInfo> &getInputInfo() const = 0;
	virtual const std::unordered_map<std::string, TensorInfo> &getOutputInfo() const = 0;
};

#ifdef USE_ONNX_ORT

class OnnxEngineOrt : public OnnxEngine {
public:
	OnnxEngineOrt();
	~OnnxEngineOrt();
	int loadModel(const std::string &modelPath) override;
	int runInference(const std::vector<float> &inputs, std::vector<float> &outputs) override;

	const std::vector<const char *> &getInputNames() const override { return inputNames_; }
	const std::vector<const char *> &getOutputNames() const override { return outputNames_; }
	bool isLoaded() const override { return session_ != nullptr; }
	const std::unordered_map<std::string, TensorInfo> &getInputInfo() const override { return inputInfo_; }
	const std::unordered_map<std::string, TensorInfo> &getOutputInfo() const override { return outputInfo_; }

private:
	Ort::Env env_{ORT_LOGGING_LEVEL_WARNING, "SoftIsp"};
	Ort::MemoryInfo memoryInfo_{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};
	Ort::SessionOptions sessionOptions_;
	Ort::Session *session_ = nullptr;
	std::vector<const char *> inputNames_;
	std::vector<const char *> outputNames_;
	std::unordered_map<std::string, TensorInfo> inputInfo_;
	std::unordered_map<std::string, TensorInfo> outputInfo_;
};

using OnnxEngineImpl = OnnxEngineOrt;

#else /* dlopen */

class OnnxEngineDlopen : public OnnxEngine {
public:
	OnnxEngineDlopen();
	~OnnxEngineDlopen();
	int loadModel(const std::string &modelPath) override;
	int runInference(const std::vector<float> &inputs, std::vector<float> &outputs) override;

	const std::vector<const char *> &getInputNames() const override { return inputNames_; }
	const std::vector<const char *> &getOutputNames() const override { return outputNames_; }
	bool isLoaded() const override { return session_ != nullptr; }
	const std::unordered_map<std::string, TensorInfo> &getInputInfo() const override { return inputInfo_; }
	const std::unordered_map<std::string, TensorInfo> &getOutputInfo() const override { return outputInfo_; }

private:
	bool initOrt();
	void *ortLib_ = nullptr;
	void *env_ = nullptr;
	void *options_ = nullptr;
	void *session_ = nullptr;
	void *memoryInfo_ = nullptr;
	std::vector<const char *> inputNames_;
	std::vector<const char *> outputNames_;
	std::unordered_map<std::string, TensorInfo> inputInfo_;
	std::unordered_map<std::string, TensorInfo> outputInfo_;
};

using OnnxEngineImpl = OnnxEngineDlopen;

#endif

} // namespace soft
} // namespace ipa
} // namespace libcamera