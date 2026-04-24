/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId,
			   const ControlList & /*sensorControls*/)
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "Not initialized";
		return;
	}

	LOG(SoftIsp, Debug) << "processStats: frame=" << frame << ", buffer=" << bufferId;

	// TODO: Read stats from SharedFD (fdStats from init)
	// TODO: Run algo.onnx inference
	// TODO: Extract AWB/AE parameters
	// TODO: Populate metadata ControlList

	ControlList metadata(0);
	metadataReady.emit(frame, metadata);

	LOG(SoftIsp, Debug) << "processStats complete, metadata emitted for frame " << frame;
}
