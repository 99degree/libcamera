/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <libcamera/ipa/ipa_interface.h>
#include <libcamera/ipa/softisp_ipa_interface.h>
#include <libcamera/base/shared_fd.h>
#include <libcamera/base/signal.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/geometry.h>
#include <libcamera/base/log.h>

#include <libcamera/ipa/softisp_ipa_interface.h>

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

class SoftIsp : public IPASoftIspInterface {
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
		     bool *ccmEnabled) override;
	int32_t start() override;
	void stop() override;
	int32_t configure(const IPAConfigInfo &configInfo) override;
	void queueRequest(const uint32_t frame, const ControlList &sensorControls) override;
	void computeParams(const uint32_t frame) override;
	void processStats(uint32_t frame, uint32_t bufferId, const libcamera::ControlList &stats) override;

	virtual int32_t configureFrameBackend(const Format format, const uint32_t num_output_buffers) override;
	void processFrame(const uint32_t frame,
			  uint32_t bufferId,
			  const SharedFD &bufferFd,
			  const int32_t planeIndexIn, const int32_t planeIndexOut,
			  const int32_t width,
			  const int32_t height,
			  const ControlList &results) override;

	std::string logPrefix() const;
	void ensureModelsLoaded();

private:
	struct Impl {
		OnnxEngineImpl algoEngine;
		OnnxEngineImpl applierEngine;
		uint32_t imageWidth = 0;
		uint32_t imageHeight = 0;
		bool initialized = false;
		SharedFD fdStats_;
		SharedFD fdParams_;
		float currentRedGain = 1.0f;
		float currentBlueGain = 1.0f;
		ControlList cachedStats;
		// Parameters computed by algo.onnx
		std::vector<float> awbGains;       // 3 gains (R,G,B)
		std::vector<float> ccmMatrix;      // 3x3 color correction matrix
		float gammaValue = 1.0f;
		float tonemapCurve = 1.0f;
		Format targetFormat = Format::RGB;
		uint32_t numOutputBuffers = 2;
		virtual ~Impl() {}
		std::vector<float> yuvMatrix;      // 3x3 RGB to YUV matrix
		std::vector<float> subsampleScale; // 4 values
	};

	std::unique_ptr<Impl> impl_;
};

extern const LogCategory &IPASoftISP;

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */