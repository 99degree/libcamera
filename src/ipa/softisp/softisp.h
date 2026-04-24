/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - ONNX-based Image Processing Algorithm
 */
#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

// ONNX Runtime headers
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra-semi"
#pragma GCC diagnostic ignored "-Wc++98-compat-extra-semi"
#include <onnxruntime_cxx_api.h>
#pragma GCC diagnostic pop

// Forward declarations (actual types provided by IPA interface)
namespace libcamera {
class SharedFD;
class ControlList;
struct IPASettings;
struct IPACameraSensorInfo;
struct IPAConfigInfo;
class ControlInfoMap;
}

namespace libcamera {
namespace ipa {
namespace soft {

// Forward declare interface
class IPASoftIspInterface;

// ONNX Runtime wrapper
class OnnxEngine {
public:
	OnnxEngine();
	~OnnxEngine();

	int loadModel(const std::string &modelPath);
	int runInference(const std::vector<float> &inputs, std::vector<float> &outputs);
	
	const std::vector<const char*> &getInputNames() const { return inputNames_; }
	const std::vector<const char*> &getOutputNames() const { return outputNames_; }
	bool isLoaded() const { return session_ != nullptr; }

	struct TensorInfo {
		std::vector<int64_t> shape;
		ONNXTensorElementDataType type;
		size_t elementCount;
	};

	const std::unordered_map<std::string, TensorInfo> &getInputInfo() const { return inputInfo_; }
	const std::unordered_map<std::string, TensorInfo> &getOutputInfo() const { return outputInfo_; }

private:
	Ort::Env env_;
	Ort::SessionOptions sessionOptions_;
	Ort::Session *session_ = nullptr;
	std::vector<const char*> inputNames_;
	std::vector<const char*> outputNames_;
	std::unordered_map<std::string, TensorInfo> inputInfo_;
	std::unordered_map<std::string, TensorInfo> outputInfo_;
	Ort::MemoryInfo memoryInfo_;
};

// SoftIsp class with dual callback pattern
class SoftIsp {
public:
	SoftIsp();
	~SoftIsp();

	// Signal for metadata completion
	libcamera::Signal<uint32_t, const ControlList &> metadataReady;
	
	// Signal for frame completion  
	libcamera::Signal<uint32_t, uint32_t> frameDone;

	// Initialization
	int32_t init(const IPASettings &settings,
	             const SharedFD &fdStats,
	             const SharedFD &fdParams,
	             const IPACameraSensorInfo &sensorInfo,
	             const ControlInfoMap &sensorControls,
	             ControlInfoMap *ipaControls,
	             bool *ccmEnabled);

	int32_t start();
	void stop();

	int32_t configure(const IPAConfigInfo &configInfo);

	void queueRequest(const uint32_t frame,
	                  const ControlList &sensorControls);

	void computeParams(const uint32_t frame);

	void processStats(const uint32_t frame,
	                  const uint32_t bufferId,
	                  const ControlList &sensorControls);

	void processFrame(const uint32_t frame,
	                  const uint32_t bufferId,
	                  const SharedFD &bufferFd,
	                  const int32_t planeIndex,
	                  const int32_t width,
	                  const int32_t height,
	                  const ControlList &results);

	std::string logPrefix() const;

private:
	struct Impl {
		OnnxEngine algoEngine;    // For stats → AWB/AE
		OnnxEngine applierEngine; // For Bayer → RGB/YUV
		uint32_t imageWidth = 0;
		uint32_t imageHeight = 0;
		bool initialized = false;
		SharedFD fdStats_;        // Stats buffer FD
		SharedFD fdParams_;       // Params buffer FD
	};

	std::unique_ptr<Impl> impl_;
};

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */
