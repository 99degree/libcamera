/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - ONNX-based Image Processing Algorithm.
 * Skeleton implementation with method includes.
 */
#pragma once

#include <libcamera/ipa/softisp_ipa_interface.h>
#include "onnx_engine.h"
#include <libcamera/ipa/ipa_module_info.h>
#include <libcamera/base/mutex.h>
#include <memory>
#include <string>
#include <vector>

namespace libcamera {
namespace ipa {
namespace soft {

/**
 * SoftIsp - IPA Module implementation for SoftISP
 * Uses ONNX Runtime for image processing.
 * Implements IPASoftIspInterface from softisp.mojom
 */
class SoftIsp : public IPASoftIspInterface
{
	struct Impl {
		Impl() = default;
		~Impl() = default;

		OnnxEngine algoEngine;      // For statistics calculation
		OnnxEngine applierEngine;   // For frame processing

		bool initialized = false;
		int imageWidth = 0;
		int imageHeight = 0;

		// ISP parameters from algo model
		std::vector<float> algoOutput;

		libcamera::Mutex mutex;
	};

public:
	SoftIsp();
	~SoftIsp() override;

	// IPASoftIspInterface methods
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
	void queueRequest(const uint32_t frame, const ControlList &sensorControls) override;
	void computeParams(const uint32_t frame) override;
	void processStats(const uint32_t frame, const uint32_t bufferId,
			  const ControlList &sensorControls) override;
	void processFrame(const uint32_t frame, const uint32_t bufferId,
			  const SharedFD &bufferFd, const int32_t planeIndex,
			  const int32_t width, const int32_t height,
			  const ControlList &results) override;

protected:
	std::string logPrefix() const;

private:
	std::unique_ptr<Impl> impl_;
};

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */
