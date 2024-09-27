/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2023, Linaro Ltd
 *
 * Simple Software Image Processing Algorithm module
 */

#include <stdint.h>
#include <sys/mman.h>

#include <linux/v4l2-controls.h>

#include <libcamera/base/file.h>
#include <libcamera/base/log.h>
#include <libcamera/base/shared_fd.h>

#include <libcamera/control_ids.h>
#include <libcamera/controls.h>

#include <libcamera/ipa/ipa_interface.h>
#include <libcamera/ipa/ipa_module_info.h>
#include <libcamera/ipa/soft_ipa_interface.h>

#include "libcamera/internal/software_isp/debayer_params.h"
#include "libcamera/internal/software_isp/swisp_stats.h"
#include "libcamera/internal/yaml_parser.h"

#include "libipa/camera_sensor_helper.h"

#include "module.h"

namespace libcamera {
LOG_DEFINE_CATEGORY(IPASoft)

namespace ipa::soft {

/*
 * The number of bins to use for the optimal exposure calculations.
 */
static constexpr unsigned int kExposureBinsCount = 5;

/*
 * The exposure is optimal when the mean sample value of the histogram is
 * in the middle of the range.
 */
static constexpr float kExposureOptimal = kExposureBinsCount / 2.0;

/*
 * The below value implements the hysteresis for the exposure adjustment.
 * It is small enough to have the exposure close to the optimal, and is big
 * enough to prevent the exposure from wobbling around the optimal value.
 */
static constexpr float kExposureSatisfactory = 0.2;
/* Maximum number of frame contexts to be held */
static constexpr uint32_t kMaxFrameContexts = 16;

class IPASoftSimple : public ipa::soft::IPASoftInterface, public Module
{
public:
	IPASoftSimple()
		: params_(nullptr), stats_(nullptr),
		  context_({ {}, {}, { kMaxFrameContexts } })
	{
	}

	~IPASoftSimple();

	int init(const IPASettings &settings,
		 const SharedFD &fdStats,
		 const SharedFD &fdParams,
		 const ControlInfoMap &sensorInfoMap) override;
	int configure(const IPAConfigInfo &configInfo) override;

	int start() override;
	void stop() override;

	void queueRequest(const uint32_t frame, const ControlList &controls) override;
	void fillParamsBuffer(const uint32_t frame) override;
	void processStats(const uint32_t frame, const uint32_t bufferId,
			  const ControlList &sensorControls) override;

protected:
	std::string logPrefix() const override;

private:
	void updateExposure(double exposureMSV);

	DebayerParams *params_;
	SwIspStats *stats_;
	std::unique_ptr<CameraSensorHelper> camHelper_;
	ControlInfoMap sensorInfoMap_;

	/* Local parameter storage */
	struct IPAContext context_;

	int32_t exposureMin_, exposureMax_;
	int32_t exposure_;
	double againMin_, againMax_, againMinStep_;
	double again_;
};

IPASoftSimple::~IPASoftSimple()
{
	if (stats_)
		munmap(stats_, sizeof(SwIspStats));
	if (params_)
		munmap(params_, sizeof(DebayerParams));
}

int IPASoftSimple::init(const IPASettings &settings,
			const SharedFD &fdStats,
			const SharedFD &fdParams,
			const ControlInfoMap &sensorInfoMap)
{
	camHelper_ = CameraSensorHelperFactoryBase::create(settings.sensorModel);
	if (!camHelper_) {
		LOG(IPASoft, Warning)
			<< "Failed to create camera sensor helper for "
			<< settings.sensorModel;
	}

	/* Load the tuning data file */
	File file(settings.configurationFile);
	if (!file.open(File::OpenModeFlag::ReadOnly)) {
		int ret = file.error();
		LOG(IPASoft, Error)
			<< "Failed to open configuration file "
			<< settings.configurationFile << ": " << strerror(-ret);
		return ret;
	}

	std::unique_ptr<libcamera::YamlObject> data = YamlParser::parse(file);
	if (!data)
		return -EINVAL;

	/* \todo Use the IPA configuration file for real. */
	unsigned int version = (*data)["version"].get<uint32_t>(0);
	LOG(IPASoft, Debug) << "Tuning file version " << version;

	if (!data->contains("algorithms")) {
		LOG(IPASoft, Error) << "Tuning file doesn't contain algorithms";
		return -EINVAL;
	}

	int ret = createAlgorithms(context_, (*data)["algorithms"]);
	if (ret)
		return ret;

	params_ = nullptr;
	stats_ = nullptr;

	if (!fdStats.isValid()) {
		LOG(IPASoft, Error) << "Invalid Statistics handle";
		return -ENODEV;
	}

	if (!fdParams.isValid()) {
		LOG(IPASoft, Error) << "Invalid Parameters handle";
		return -ENODEV;
	}

	{
		void *mem = mmap(nullptr, sizeof(DebayerParams), PROT_WRITE,
				 MAP_SHARED, fdParams.get(), 0);
		if (mem == MAP_FAILED) {
			LOG(IPASoft, Error) << "Unable to map Parameters";
			return -errno;
		}

		params_ = static_cast<DebayerParams *>(mem);
	}

	{
		void *mem = mmap(nullptr, sizeof(SwIspStats), PROT_READ,
				 MAP_SHARED, fdStats.get(), 0);
		if (mem == MAP_FAILED) {
			LOG(IPASoft, Error) << "Unable to map Statistics";
			return -errno;
		}

		stats_ = static_cast<SwIspStats *>(mem);
	}

	/*
	 * Check if the sensor driver supports the controls required by the
	 * Soft IPA.
	 * Don't save the min and max control values yet, as e.g. the limits
	 * for V4L2_CID_EXPOSURE depend on the configured sensor resolution.
	 */
	if (sensorInfoMap.find(V4L2_CID_EXPOSURE) == sensorInfoMap.end()) {
		LOG(IPASoft, Error) << "Don't have exposure control";
		return -EINVAL;
	}

	if (sensorInfoMap.find(V4L2_CID_ANALOGUE_GAIN) == sensorInfoMap.end()) {
		LOG(IPASoft, Error) << "Don't have gain control";
		return -EINVAL;
	}

	return 0;
}

int IPASoftSimple::configure(const IPAConfigInfo &configInfo)
{
	sensorInfoMap_ = configInfo.sensorControls;

	const ControlInfo &exposureInfo = sensorInfoMap_.find(V4L2_CID_EXPOSURE)->second;
	const ControlInfo &gainInfo = sensorInfoMap_.find(V4L2_CID_ANALOGUE_GAIN)->second;

	exposureMin_ = exposureInfo.min().get<int32_t>();
	exposureMax_ = exposureInfo.max().get<int32_t>();
	if (!exposureMin_) {
		LOG(IPASoft, Warning) << "Minimum exposure is zero, that can't be linear";
		exposureMin_ = 1;
	}

	int32_t againMin = gainInfo.min().get<int32_t>();
	int32_t againMax = gainInfo.max().get<int32_t>();

	if (camHelper_) {
		againMin_ = camHelper_->gain(againMin);
		againMax_ = camHelper_->gain(againMax);
		againMinStep_ = (againMax_ - againMin_) / 100.0;
	} else {
		/*
		 * The camera sensor gain (g) is usually not equal to the value written
		 * into the gain register (x). But the way how the AGC algorithm changes
		 * the gain value to make the total exposure closer to the optimum
		 * assumes that g(x) is not too far from linear function. If the minimal
		 * gain is 0, the g(x) is likely to be far from the linear, like
		 * g(x) = a / (b * x + c). To avoid unexpected changes to the gain by
		 * the AGC algorithm (abrupt near one edge, and very small near the
		 * other) we limit the range of the gain values used.
		 */
		againMax_ = againMax;
		if (!againMin) {
			LOG(IPASoft, Warning)
				<< "Minimum gain is zero, that can't be linear";
			againMin_ = std::min(100, againMin / 2 + againMax / 2);
		}
		againMinStep_ = 1.0;
	}

	for (auto const &algo : algorithms()) {
		int ret = algo->configure(context_, configInfo);
		if (ret)
			return ret;
	}

	LOG(IPASoft, Info) << "Exposure " << exposureMin_ << "-" << exposureMax_
			   << ", gain " << againMin_ << "-" << againMax_
			   << " (" << againMinStep_ << ")";

	return 0;
}

int IPASoftSimple::start()
{
	return 0;
}

void IPASoftSimple::stop()
{
}

void IPASoftSimple::queueRequest(const uint32_t frame, const ControlList &controls)
{
	IPAFrameContext &frameContext = context_.frameContexts.alloc(frame);

	for (auto const &algo : algorithms())
		algo->queueRequest(context_, frame, frameContext, controls);
}

void IPASoftSimple::fillParamsBuffer(const uint32_t frame)
{
	IPAFrameContext &frameContext = context_.frameContexts.get(frame);
	for (auto const &algo : algorithms())
		algo->prepare(context_, frame, frameContext, params_);
	setIspParams.emit();
}

void IPASoftSimple::processStats(const uint32_t frame,
				 [[maybe_unused]] const uint32_t bufferId,
				 const ControlList &sensorControls)
{
	IPAFrameContext &frameContext = context_.frameContexts.get(frame);
	/*
	 * Software ISP currently does not produce any metadata. Use an empty
	 * ControlList for now.
	 *
	 * \todo Implement proper metadata handling
	 */
	ControlList metadata(controls::controls);
	for (auto const &algo : algorithms())
		algo->process(context_, frame, frameContext, stats_, metadata);

	/* \todo Switch to the libipa/algorithm.h API someday. */

	/*
	 * Calculate Mean Sample Value (MSV) according to formula from:
	 * https://www.araa.asn.au/acra/acra2007/papers/paper84final.pdf
	 */
	const uint8_t blackLevel = context_.activeState.blc.level;
	const unsigned int blackLevelHistIdx =
		blackLevel / (256 / SwIspStats::kYHistogramSize);
	const unsigned int histogramSize =
		SwIspStats::kYHistogramSize - blackLevelHistIdx;
	const unsigned int yHistValsPerBin = histogramSize / kExposureBinsCount;
	const unsigned int yHistValsPerBinMod =
		histogramSize / (histogramSize % kExposureBinsCount + 1);
	int exposureBins[kExposureBinsCount] = {};
	unsigned int denom = 0;
	unsigned int num = 0;

	for (unsigned int i = 0; i < histogramSize; i++) {
		unsigned int idx = (i - (i / yHistValsPerBinMod)) / yHistValsPerBin;
		exposureBins[idx] += stats_->yHistogram[blackLevelHistIdx + i];
	}

	for (unsigned int i = 0; i < kExposureBinsCount; i++) {
		LOG(IPASoft, Debug) << i << ": " << exposureBins[i];
		denom += exposureBins[i];
		num += exposureBins[i] * (i + 1);
	}

	float exposureMSV = static_cast<float>(num) / denom;

	/* Sanity check */
	if (!sensorControls.contains(V4L2_CID_EXPOSURE) ||
	    !sensorControls.contains(V4L2_CID_ANALOGUE_GAIN)) {
		LOG(IPASoft, Error) << "Control(s) missing";
		return;
	}

	exposure_ = sensorControls.get(V4L2_CID_EXPOSURE).get<int32_t>();
	int32_t again = sensorControls.get(V4L2_CID_ANALOGUE_GAIN).get<int32_t>();
	again_ = camHelper_ ? camHelper_->gain(again) : again;

	updateExposure(exposureMSV);

	ControlList ctrls(sensorInfoMap_);

	ctrls.set(V4L2_CID_EXPOSURE, exposure_);
	ctrls.set(V4L2_CID_ANALOGUE_GAIN,
		  static_cast<int32_t>(camHelper_ ? camHelper_->gainCode(again_) : again_));

	setSensorControls.emit(ctrls);

	LOG(IPASoft, Debug) << "exposureMSV " << exposureMSV
			    << " exp " << exposure_ << " again " << again_
			    << " black level " << static_cast<unsigned int>(blackLevel);
}

void IPASoftSimple::updateExposure(double exposureMSV)
{
	/*
	 * kExpDenominator of 10 gives ~10% increment/decrement;
	 * kExpDenominator of 5 - about ~20%
	 */
	static constexpr uint8_t kExpDenominator = 10;
	static constexpr uint8_t kExpNumeratorUp = kExpDenominator + 1;
	static constexpr uint8_t kExpNumeratorDown = kExpDenominator - 1;

	double next;

	if (exposureMSV < kExposureOptimal - kExposureSatisfactory) {
		next = exposure_ * kExpNumeratorUp / kExpDenominator;
		if (next - exposure_ < 1)
			exposure_ += 1;
		else
			exposure_ = next;
		if (exposure_ >= exposureMax_) {
			next = again_ * kExpNumeratorUp / kExpDenominator;
			if (next - again_ < againMinStep_)
				again_ += againMinStep_;
			else
				again_ = next;
		}
	}

	if (exposureMSV > kExposureOptimal + kExposureSatisfactory) {
		if (exposure_ == exposureMax_ && again_ > againMin_) {
			next = again_ * kExpNumeratorDown / kExpDenominator;
			if (again_ - next < againMinStep_)
				again_ -= againMinStep_;
			else
				again_ = next;
		} else {
			next = exposure_ * kExpNumeratorDown / kExpDenominator;
			if (exposure_ - next < 1)
				exposure_ -= 1;
			else
				exposure_ = next;
		}
	}

	exposure_ = std::clamp(exposure_, exposureMin_, exposureMax_);
	again_ = std::clamp(again_, againMin_, againMax_);
}

std::string IPASoftSimple::logPrefix() const
{
	return "IPASoft";
}

} /* namespace ipa::soft */

/*
 * External IPA module interface
 */
extern "C" {
const struct IPAModuleInfo ipaModuleInfo = {
	IPA_MODULE_API_VERSION,
	0,
	"simple",
	"simple",
};

IPAInterface *ipaCreate()
{
	return new ipa::soft::IPASoftSimple();
}

} /* extern "C" */

} /* namespace libcamera */
