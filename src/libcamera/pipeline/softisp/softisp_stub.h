/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftISP Stub Interface - Simple interface for pipeline to use SoftIsp
 */
#pragma once

#include <libcamera/ipa/ipa_interface.h>

namespace libcamera {
namespace pipeline {
namespace softisp {

/**
 * SoftISPStub - Simple wrapper around ipa::soft::SoftIsp
 * This allows the pipeline to use SoftIsp without including ONNX headers
 */
class SoftISPStub : public IPAInterface
{
public:
	SoftISPStub();
	~SoftISPStub() override;

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

protected:
	std::string logPrefix() const override;

private:
	class Impl;
	std::unique_ptr<Impl> impl_;
};

} /* namespace softisp */
} /* namespace pipeline */
} /* namespace libcamera */
