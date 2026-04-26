/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

int SoftISPCameraData::exportFrameBuffers(
	[[maybe_unused]] const Stream *stream,
	std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	LOG(SoftISPPipeline, Info) << "Exporting frame buffers";
	if (!frameGenerator_) {
		LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
		return -EINVAL;
	}

	/*
	 * Clone the buffers from VirtualCamera. The Camera framework takes
	 * ownership of the returned unique_ptr and manages them.
	 * VirtualCamera keeps its own reference via buffers_.
	 */
	auto &genBuffers = frameGenerator_->getBuffers();
	for (auto *buffer : genBuffers) {
		if (!buffer)
			continue;
		if (buffer->planes().empty())
			continue;

		/* Clone the plane data so the Camera framework gets its own fd */
		std::vector<FrameBuffer::Plane> planes;
		for (const auto &plane : buffer->planes()) {
			int clonedFd = dup(plane.fd.get());
			if (clonedFd < 0) {
				LOG(SoftISPPipeline, Error) << "Failed to dup fd";
				return -errno;
			}
			FrameBuffer::Plane clonedPlane;
			clonedPlane.fd = SharedFD(clonedFd);
			clonedPlane.length = plane.length;
			clonedPlane.offset = plane.offset;
			planes.push_back(std::move(clonedPlane));
		}

		auto *cloned = new FrameBuffer(planes);
		buffers->push_back(std::unique_ptr<FrameBuffer>(cloned));
	}

	LOG(SoftISPPipeline, Info) << "Exported " << buffers->size() << " frame buffers";
	return 0;
}