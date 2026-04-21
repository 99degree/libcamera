/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * SoftISP IPA Module for Virtual-SoftISP pipeline
 *
 * This module is identical to softisp_module.cpp but has a different
 * pipelineName to match the "dummysoftisp" pipeline handler.
 */

#include "softisp.h"
#include "softisp_module.h"

#include <libcamera/base/log.h>
#include <libcamera/ipa/ipa_interface.h>
#include <libcamera/ipa/ipa_module_info.h>
#include <libcamera/ipa/soft_ipa_interface.h>

namespace libcamera {
namespace soft {

/*
 * SoftISP Module class for Virtual-SoftISP pipeline.
 *
 * This is a wrapper around the SoftIsp algorithm, providing the
 * IPASoftInterface implementation required by the pipeline.
 */
class SoftISPVirtualModule : public ipa::soft::IPASoftInterface
{
public:
	SoftISPVirtualModule();
	~SoftISPVirtualModule() override;

	int init(Context &context, const std::string &dataPath) override;
	int configure(const IPAConfiguration &config) override;
	void process(IPAContext &context, const uint32_t frame,
		     IPAFrameContext &frameContext,
		     const IPAStatistics *stats, ControlList &metadata) override;
	int queueBuffer(const uint32_t frame, const unsigned int planeId,
			const struct dma_buf *buf) override;

private:
	SoftIsp algo_;
	bool initialized_ = false;
};

SoftISPVirtualModule::SoftISPVirtualModule()
{
}

SoftISPVirtualModule::~SoftISPVirtualModule()
{
}

int SoftISPVirtualModule::init(Context &context, const std::string &dataPath)
{
	int ret = algo_.init(context, dataPath);
	if (ret)
		return ret;

	initialized_ = true;
	return 0;
}

int SoftISPVirtualModule::configure(const IPAConfiguration &config)
{
	if (!initialized_)
		return -EINVAL;

	return algo_.configure(config);
}

void SoftISPVirtualModule::process(IPAContext &context, const uint32_t frame,
				   IPAFrameContext &frameContext,
				   const IPAStatistics *stats,
				   ControlList &metadata)
{
	if (!initialized_)
		return;

	algo_.process(context, frame, frameContext, stats, metadata);
}

int SoftISPVirtualModule::queueBuffer(const uint32_t frame,
				      const unsigned int planeId,
				      const struct dma_buf *buf)
{
	/* Placeholder: Not used by SoftISP */
	return 0;
}

} /* namespace soft */
} /* namespace libcamera */

/* -----------------------------------------------------------------------------
 * IPA Module Info
 * ---------------------------------------------------------------------------*/

extern "C" {

/*
 * Module info for the Virtual-SoftISP pipeline.
 *
 * Note: The pipelineName is set to "dummysoftisp" to match the
 * dummysoftisp pipeline handler, unlike the regular softisp module
 * which uses "softisp".
 */
const struct IPAModuleInfo ipaModuleInfo = {
	IPA_MODULE_API_VERSION,
	0,
	"softisp-virtual",  /* Module name */
	"dummysoftisp",  /* Pipeline name - matches "dummysoftisp" handler */
};

IPAInterface *ipaCreate()
{
	return new libcamera::soft::SoftISPVirtualModule();
}

} /* extern "C" */
