/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * \file af.h
 * \brief Auto-Focus IPA Algorithm for Simple Pipeline
 *
 * This algorithm integrates the hardware-agnostic AfAlgo class into
 * the simple IPA pipeline. It calculates focus metrics from image data
 * and outputs lens position controls.
 */
#pragma once

#include <memory>
#include <libcamera/control_list.h>
#include <libcamera/ipa/algorithm.h>

#include "libipa/af_algo.h"
#include "libipa/af_stats.h"
#include "simple/ipa_context.h"

namespace libcamera {
namespace ipa::soft::algorithms {

class Af : public Algorithm {
public:
	Af();
	~Af() override;

	int configure(IPAContext &context, const IPAConfigInfo &configInfo) override;
	void prepare(IPAContext &context, const uint32_t frame,
	             IPAFrameContext &frameContext,
	             DebayerParams *params) override;
	void process(IPAContext &context, const uint32_t frame,
	             IPAFrameContext &frameContext,
	             const SwIspStats *stats, ControlList &metadata) override;

private:
	std::unique_ptr<libipa::AfAlgo> afAlgo_;
	std::unique_ptr<libipa::AfStatsCalculator> statsCalculator_;

	bool initialized_;
	uint32_t frameCount_;
	bool lensPositionUpdated_;
	int32_t lastLensPosition_;
};

} // namespace ipa::soft::algorithms
} // namespace libcamera
