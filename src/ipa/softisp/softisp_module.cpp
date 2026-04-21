/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * SoftISP IPA Module - Software ISP using ONNX models
 */

#include "softisp_module.h"

#include <libcamera/base/log.h>
#include <libcamera/ipa/ipa_module_info.h>

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPModule)

namespace ipa {
namespace softisp {

SoftISPModule::SoftISPModule()
	: context_(16) /* Max frame contexts */
{
	LOG(SoftISPModule, Debug) << "SoftISP Module created";
	softIsp_ = std::make_unique<SoftIsp>();
}

SoftISPModule::~SoftISPModule()
{
	softIsp_.reset();
	LOG(SoftISPModule, Debug) << "SoftISP Module destroyed";
}

int SoftISPModule::init(const IPASettings &settings,
			const SharedFD &fdStats,
			const SharedFD &fdParams,
			const IPACameraSensorInfo &sensorInfo,
			const ControlInfoMap &sensorControls,
			ControlInfoMap *ipaControls,
			bool *ccmEnabled)
{
	LOG(SoftISPModule, Debug) << "Initializing SoftISP Module";
	
	// Initialize the SoftISP algorithm
	int ret = softIsp_->init(context_, {});
	if (ret < 0) {
		LOG(SoftISPModule, Error) << "Failed to initialize SoftISP algorithm";
		return ret;
	}
	
	// Configure the algorithm with sensor info
	// TODO: Pass appropriate configuration data
	ret = softIsp_->configure(context_, {});
	if (ret < 0) {
		LOG(SoftISPModule, Error) << "Failed to configure SoftISP algorithm";
		return ret;
	}
	
	if (ipaControls) {
		// Add controls supported by SoftISP
		// For now, just add colour gains
		*ipaControls = sensorControls;
		ipaControls->set(control_id::colourGains, ControlList::Value({1.0f, 1.0f}));
	}
	
	if (ccmEnabled)
		*ccmEnabled = false;
	
	return 0;
}

int SoftISPModule::configure(const IPAConfigInfo &configInfo)
{
	LOG(SoftISPModule, Debug) << "Configuring SoftISP Module";
	// The algorithm is already configured in init()
	return 0;
}

int SoftISPModule::start()
{
	LOG(SoftISPModule, Debug) << "Starting SoftISP Module";
	return 0;
}

void SoftISPModule::stop()
{
	LOG(SoftISPModule, Debug) << "Stopping SoftISP Module";
	softIsp_->reset();
}

void SoftISPModule::queueRequest(const uint32_t frame,
				 const ControlList &controls)
{
	// Forward to algorithm if needed
	softIsp_->queueRequest(context_, frame, controls);
}

void SoftISPModule::computeParams(const uint32_t frame)
{
	// Forward to algorithm if needed
	softIsp_->prepare(context_, frame, {});
}

void SoftISPModule::processStats(const uint32_t frame,
				 const uint32_t bufferId,
				 const ControlList &sensorControls)
{
	// This is where the main processing happens
	// Get the statistics for this frame
	// TODO: Retrieve actual statistics from the pipeline
	const IPAStatistics *stats = nullptr; // Placeholder
	
	// Call the SoftISP algorithm to process
	ControlList metadata;
	softIsp_->process(context_, frame, {}, stats, metadata);
	
	// TODO: Apply the metadata to the frame
}

std::string SoftISPModule::logPrefix() const
{
	return "SoftISP";
}

} /* namespace softisp */
} /* namespace ipa */

/* IPA Module Info */
const struct IPAModuleInfo ipaModuleInfo = {
	IPA_MODULE_API_VERSION,
	0,
	"softisp",      /* Module name */
	"simple",       /* Pipeline name - matches "simple" pipeline handler */
};

/* IPA Module Creation Function */
extern "C" {
ipa::soft::IPASoftInterface *ipaCreate()
{
	return new libcamera::ipa::softisp::SoftISPModule();
}
}

} /* namespace libcamera */
