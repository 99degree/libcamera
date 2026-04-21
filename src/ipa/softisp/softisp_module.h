/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * SoftISP IPA Module - Software ISP using ONNX models
 *
 * This module implements the IPASoftInterface and contains
 * the SoftISP algorithm for two-stage ONNX inference.
 */
#pragma once

#include <libcamera/ipa/soft_ipa_interface.h>
#include <memory>
#include "softisp.h"

namespace libcamera {
namespace ipa {
namespace softisp {

class SoftISPModule : public ipa::soft::IPASoftInterface
{
public:
	SoftISPModule();
	~SoftISPModule() override;

	int init(const IPASettings &settings,
		 const SharedFD &fdStats,
		 const SharedFD &fdParams,
		 const IPACameraSensorInfo &sensorInfo,
		 const ControlInfoMap &sensorControls,
		 ControlInfoMap *ipaControls,
		 bool *ccmEnabled) override;

	int configure(const IPAConfigInfo &configInfo) override;

	int start() override;

	void stop() override;

	void queueRequest(const uint32_t frame,
			  const ControlList &controls) override;

	void computeParams(const uint32_t frame) override;

	void processStats(const uint32_t frame,
			  const uint32_t bufferId,
			  const ControlList &sensorControls) override;

protected:
	std::string logPrefix() const override;

private:
	IPAContext context_;
	std::unique_ptr<SoftIsp> softIsp_;
};

} /* namespace softisp */
} /* namespace ipa */
} /* namespace libcamera */
