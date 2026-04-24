/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftIsp::processFrame(const uint32_t frame, const uint32_t bufferId,
			   const SharedFD &bufferFd, const int32_t /*planeIndex*/,
			   const int32_t width, const int32_t height,
			   const ControlList & /*results*/)
{
	if (!impl_->initialized) {
		LOG(SoftIsp, Warning) << "Not initialized";
		return;
	}

	LOG(SoftIsp, Debug) << "processFrame: frame=" << frame 
	                    << ", buffer=" << bufferId
	                    << ", size=" << width << "x" << height;

	// TODO: Read Bayer frame from bufferFd (SBGGR10)
	// TODO: Apply AWB/AE parameters from algoOutput
	// TODO: Run applier.onnx inference
	// TODO: Convert Bayer to RGB/YUV
	// TODO: Write output back to bufferFd (or to separate output buffer)

	// Signal completion (Stage 2 done)
	// This tells the Pipeline: "I'm done with this buffer, you can reuse/free it"
	frameDone.emit(frame, bufferId);

	LOG(SoftIsp, Debug) << "processFrame complete for frame " << frame;
}
