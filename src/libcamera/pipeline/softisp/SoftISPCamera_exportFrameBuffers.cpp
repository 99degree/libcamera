/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

namespace libcamera {

int SoftISPCameraData::exportFrameBuffers(Stream *stream, std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
	LOG(SoftISPPipeline, Info) << "=== exportFrameBuffers() called ===";
	LOG(SoftISPPipeline, Info) << "Stream pointer: " << stream;
	LOG(SoftISPPipeline, Info) << "Buffers pointer: " << buffers;
	LOG(SoftISPPipeline, Info) << "virtualCamera_ pointer: " << virtualCamera_.get();
	
	if (!buffers) {
		LOG(SoftISPPipeline, Error) << "ERROR: Null buffers vector";
		return -EINVAL;
	}
	
	if (!virtualCamera_) {
		LOG(SoftISPPipeline, Error) << "ERROR: VirtualCamera not initialized";
		return -EINVAL;
	}
	
	LOG(SoftISPPipeline, Info) << "Checks passed, proceeding with buffer creation";
	
	// Get buffer count from VirtualCamera
	unsigned int count = virtualCamera_->bufferCount();
	unsigned int width = virtualCamera_->width();
	unsigned int height = virtualCamera_->height();
	
	// Calculate buffer size for Bayer10 (10 bits per pixel, packed)
	size_t bufferSize = (width * height * 5) / 4;
	
	LOG(SoftISPPipeline, Info) << "Creating " << count << " buffers of size " << bufferSize << " for " << width << "x" << height;
	
	buffers->clear();
	
	for (unsigned int i = 0; i < count; ++i) {
		int fd = -1;
		
#ifdef __linux__
		fd = memfd_create("softisp_buffer", MFD_CLOEXEC);
#endif
		if (fd < 0) {
			LOG(SoftISPPipeline, Info) << "memfd_create failed, using /dev/zero for buffer " << i;
			fd = open("/dev/zero", O_RDWR | O_CLOEXEC);
		}
		
		if (fd < 0) {
			LOG(SoftISPPipeline, Error) << "Failed to create buffer " << i << ": " << strerror(errno);
			return -errno;
		}
		
		if (ftruncate(fd, bufferSize) < 0) {
			LOG(SoftISPPipeline, Error) << "ftruncate failed for buffer " << i;
			close(fd);
			return -errno;
		}
		
		FrameBuffer::Plane plane;
		plane.fd = SharedFD(fd);
		plane.offset = 0;
		plane.length = bufferSize;
		
		std::vector<FrameBuffer::Plane> planes;
		planes.push_back(std::move(plane));
		
		auto buffer = std::make_unique<FrameBuffer>(
			Span<const FrameBuffer::Plane>(planes),
			static_cast<unsigned int>(i)
		);
		
		uint32_t bufferId = static_cast<uint32_t>(buffers->size());
		storeBuffer(bufferId, buffer.get());
		
		buffers->push_back(std::move(buffer));
		
		LOG(SoftISPPipeline, Debug) << "Created buffer " << i << " (FD: " << fd << ", Size: " << bufferSize << ")";
	}
	
	LOG(SoftISPPipeline, Info) << "Successfully exported " << buffers->size() << " frame buffers";
	return 0;
}

} /* namespace libcamera */
