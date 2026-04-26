/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <libcamera/ipa/ipa_interface.h>
#include <libcamera/ipa/soft_ipa_interface.h>
#include <libcamera/base/shared_fd.h>
#include <libcamera/base/signal.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/geometry.h>
#include <libcamera/base/log.h>

#include "onnx_engine.h"

#include <memory>
#include <string>
#include <vector>

using ::libcamera::ControlList;
using ::libcamera::SharedFD;
using ::libcamera::ControlInfoMap;

namespace libcamera {
namespace ipa {
namespace soft {

class IPASoftIspInterface;

class SoftIsp : public IPASoftInterface {
public:
	SoftIsp();
	~SoftIsp();

	::libcamera::Signal<uint32_t, const ControlList &> metadataReady;
	::libcamera::Signal<uint32_t, uint32_t> frameDone;

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
	void queueRequest(const uint32_t frame, const ControlList &sensorControls);
	void computeParams(const uint32_t frame);
	void processStats(uint32_t frame, uint32_t bufferId, libcamera::ControlList &stats);
	void processFrame(const uint32_t frame,
			  uint32_t bufferId,
			  const SharedFD &bufferFd,
			  const int32_t planeIndex,
			  const int32_t width,
			  const int32_t height,
			  const ControlList &results);

	std::string logPrefix() const;
	void ensureModelsLoaded();

private:
	struct Impl {
		std::unique_ptr<OnnxEngine> algoEngine;
		std::unique_ptr<OnnxEngine> applierEngine;
		uint32_t imageWidth = 0;
		uint32_t imageHeight = 0;
		bool initialized = false;
		SharedFD fdStats_;
		SharedFD fdParams_;
		float currentRedGain = 1.0f;
		float currentBlueGain = 1.0f;
		ControlList cachedStats;
	};

	std::unique_ptr<Impl> impl_;
};

extern const LogCategory &IPASoftISP;

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */