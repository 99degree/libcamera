/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - ONNX-based Image Processing Algorithm.
 * Stub version without ONNX runtime dependency.
 */
#pragma once

#include "libcamera/ipa/soft_ipa_interface.h"
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
 * Inherits from IPASoftInterface (generated from MOJOM)
 */
class SoftIsp : public IPASoftInterface
{
	struct Impl {
		Impl() = default;
		~Impl() = default;
		
		// Stub members (ONNX members commented out)
		// Ort::MemoryInfo memoryInfo;
		// Ort::AllocatorWithDefaultOptions allocator;
		bool initialized = false;
		int imageWidth = 640;
		int imageHeight = 480;
		std::string algoModelPath;
		std::string applierModelPath;
		std::vector<const char*> algoInputNames;
		std::vector<const char*> algoOutputNames;
		std::vector<const char*> applierInputNames;
		std::vector<const char*> applierOutputNames;
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
	void queueRequest(uint32_t frame, const ControlList &controls) override;
	void computeParams(uint32_t frame) override;
	void processStats(uint32_t frame,
			  uint32_t bufferId,
			  ControlList &stats) override;

	// Extended method for pipeline integration

protected:
	std::string logPrefix() const;

private:
	std::unique_ptr<Impl> impl_;
};

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */
