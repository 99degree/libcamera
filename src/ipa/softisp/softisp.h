/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - Image Processing Algorithm that runs two ONNX models.
 */
#include <onnxruntime_cxx_api.h>
#pragma once

#include "module.h"
#include <libcamera/ipa/soft_ipa_interface.h>
#include <libcamera/ipa/ipa_module_info.h>
#include <string>

namespace libcamera {
namespace ipa::soft {

/**
 * SoftIsp - IPA Module implementation for SoftISP
 * Inherits from both the generated interface and Module base class
 */
class SoftIsp : public IPASoftInterface, public Module {
public:
	SoftIsp();
	~SoftIsp() override;

	// IPASoftInterface methods (all pure virtual)
	int32_t init(const IPASettings &settings, const SharedFD &fdStats, const SharedFD &fdParams,
	             const IPACameraSensorInfo &sensorInfo, const ControlInfoMap &sensorControls,
	             ControlInfoMap *ipaControls, bool *ccmEnabled) override;
	int32_t start() override;
	void stop() override;
	int32_t configure(const IPAConfigInfo &configInfo) override;
	void queueRequest(const uint32_t frame, const ControlList &sensorControls) override;
	void computeParams(const uint32_t frame) override;
	void processStats(const uint32_t frame, const uint32_t bufferId, const ControlList &sensorControls) override;

protected:
	std::string logPrefix() const override;

private:
	struct Impl;
	std::unique_ptr<Impl> impl_;
};

} // namespace ipa::soft
} // namespace libcamera
