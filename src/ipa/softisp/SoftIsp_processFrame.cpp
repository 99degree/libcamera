/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <libcamera/base/log.h>

void SoftIsp::processFrame(const uint32_t frame,
			   uint32_t bufferId,
			   const SharedFD &bufferFd,
			   const int32_t planeIndex,
			   const int32_t width,
			   const int32_t height,
			   [[maybe_unused]] const ControlList &results)
{
	LOG(SoftIsp, Info) << "processFrame: frame=" << frame
			   << ", bufferId=" << bufferId
			   << ", size=" << width << "x" << height;

	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "processFrame called before init";
		return;
	}

	/* Lazy-load ONNX models on first frame */
	ensureModelsLoaded();

	/* 
	 * ISP processing complete.
	 * Emit metadataReady with empty controls (no libipa yet).
	 * This is the dispatch signal: ISP → Pipeline.
	 */
	ControlList metadata;
	metadataReady.emit(frame, metadata);

	/*
	 * Emit frameDone as the ISP's last resort.
	 * This notifies the pipeline the frame is ready for the application.
	 */
	frameDone.emit(frame, bufferId);

	LOG(SoftIsp, Info) << "processFrame: emitted metadataReady + frameDone for frame " << frame;
}