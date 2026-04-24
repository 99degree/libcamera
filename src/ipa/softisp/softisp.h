/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - ONNX-based Image Processing Algorithm
 * 
 * Architecture: Async Processing with Dual Callbacks
 * 
 * Callbacks:
 * 1. metadataReady(frameId, metadata) - Stats processing complete (AWB/AE)
 * 2. frameDone(frameId, bufferId) - Frame processing complete (output written)
 * 
 * Both callbacks must be received before Pipeline completes the request.
 */
#pragma once

#include <memory>
#include <string>
#include <libcamera/base/signal.h>
#include <libcamera/base/sharedfd.h>
#include <libcamera/base/utils.h>
#include <libcamera/control_list.h>
#include <libcamera/ipa/ipa.h>
#include <libcamera/ipa/ipa_interface.h>
#include <libcamera/ipa/ipa_settings.h>
#include <libcamera/ipa/ipa_config_info.h>
#include <libcamera/ipa/ipa_camera_sensor_info.h>

namespace libcamera {
namespace ipa {
namespace soft {

// ONNX Runtime wrapper
class OnnxEngine {
public:
	OnnxEngine();
	~OnnxEngine();

	int loadModel(const std::string &modelPath);
	int runInference(const float *input, size_t inputSize,
	                 float *output, size_t outputSize);
	
private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

// SoftIsp class with dual callback pattern
class SoftIsp : public IPAInterface<SoftIsp, IPASoftIspInterface> {
public:
	SoftIsp();
	~SoftIsp();

	// IPAInterface methods
	int32_t init(const IPASettings &settings,
	             const SharedFD &fdStats,
	             const SharedFD &fdParams,
	             const IPACameraSensorInfo &sensorInfo,
	             const ControlInfoMap &sensorControls,
	             ControlInfoMap *ipaControls,
	             bool *ccmEnabled) override;

	int32_t start() override;
	void stop() override;

	int32_t configure(const IPAConfigInfo &configInfo) override;

	void queueRequest(const uint32_t frame,
	                  const ControlList &sensorControls) override;

	void computeParams(const uint32_t frame) override;

	void processStats(const uint32_t frame,
	                  const uint32_t bufferId,
	                  const ControlList &sensorControls) override;

	void processFrame(const uint32_t frame,
	                  const uint32_t bufferId,
	                  const SharedFD &bufferFd,
	                  const int32_t planeIndex,
	                  const int32_t width,
	                  const int32_t height,
	                  const ControlList &results) override;

	std::string logPrefix() const override;

private:
	struct Impl {
		OnnxEngine algoEngine;   // For stats → AWB/AE
		OnnxEngine applierEngine; // For Bayer → RGB/YUV
		uint32_t imageWidth = 0;
		uint32_t imageHeight = 0;
		bool initialized = false;
	};

	std::unique_ptr<Impl> impl_;
};

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */

// Add frameDone signal to IPASoftIspInterface
namespace ipa {
namespace soft {

// Extend the interface with frameDone callback
class IPASoftIspInterface : public IPAInterfaceBase {
public:
	// Existing signals
	virtual Signal<uint32_t, const ControlList &> &metadataReady() = 0;
	
	// NEW: Frame completion signal
	virtual Signal<uint32_t, uint32_t> &frameDone() = 0;
};

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */
