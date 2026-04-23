/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - ONNX-based Image Processing Algorithm.
 */
#pragma once

#include "libcamera/ipa/soft_ipa_interface.h"
#include "onnx_engine.h"
#include <libcamera/ipa/ipa_module_info.h>
#include <libcamera/base/mutex.h>
#include <memory>
#include <string>

namespace libcamera {
namespace ipa {
namespace soft {

/**
 * SoftIsp - IPA Module implementation for SoftISP
 * Uses ONNX Runtime for image processing.
 */
class SoftIsp : public IPASoftInterface
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

	// IPASoftInterface methods
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
	void queueRequest(const uint32_t frame, const ControlList &controls) override;
	void computeParams(const uint32_t frame) override;
	void processStats(const uint32_t frame,
			  const uint32_t bufferId,
			  ControlList &stats) override;

	// Extended method for pipeline integration
	void processFrame(const uint32_t frameId, const uint32_t bufferId,
			  const SharedFD &bufferFd, const uint32_t offset,
			  const uint32_t width, const uint32_t height,
			  ControlList *results);

protected:
	std::string logPrefix() const;

private:
	std::unique_ptr<Impl> impl_;
};

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */
