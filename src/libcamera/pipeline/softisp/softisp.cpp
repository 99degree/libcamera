/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftISP Pipeline Handler
 * 
 * Architecture: Caller (Pipeline) / Callee (IPA) Pattern
 * 
 * CALLER (Pipeline) Responsibilities:
 * - Pre-calculate buffer sizes based on stream configuration
 * - Allocate FrameBuffers with correct sizes
 * - Map buffers via mmap() for direct memory access
 * - Pass file descriptors (SharedFD) to IPA
 * - Handle cleanup (munmap, deallocation)
 * 
 * CALLEE (IPA) Responsibilities:
 * - Stateless processing (no internal state between calls)
 * - Read from pre-mapped buffers
 * - Process data (ONNX inference)
 * - Write results back to pre-mapped buffers
 * - No memory allocation during processing
 */
#include "softisp.h"
#include "virtual_camera.h"
#include <algorithm>
#include <cstring>
#include <fcntl.h>
#include <memory>
#include <queue>
#include <sys/mman.h>
#include <unistd.h>
#include <libcamera/base/log.h>
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/geometry.h>
#include "libcamera/internal/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/framebuffer.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"

namespace libcamera {
LOG_DEFINE_CATEGORY(SoftISPPipeline)

// ... (existing PipelineHandlerSoftISP, SoftISPConfiguration implementations)

// SoftISPCameraData implementation with proper buffer setup
SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
	: Camera::Private(pipe), Thread("SoftISPCamera")
{
	virtualCamera_ = std::make_unique<VirtualCamera>();
}

SoftISPCameraData::~SoftISPCameraData()
{
	Thread::exit(0);
	wait();
}

int SoftISPCameraData::init()
{
	int ret = loadIPA();
	if (ret)
		return ret;

	if (isVirtualCamera) {
		ret = virtualCamera_->init(1920, 1080);
		if (ret) {
			LOG(SoftISPPipeline, Error) << "Failed to initialize virtual camera";
			return ret;
		}
	}

	return 0;
}

int SoftISPCameraData::loadIPA()
{
	// Try to load IPA module first
	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(
		this->pipe(), 0, 0);
	if (!ipa_) {
		LOG(SoftISPPipeline, Info) << "IPA module not available, using SoftIsp directly";
		try {
			ipa_ = new ipa::soft::SoftIsp();
			LOG(SoftISPPipeline, Info) << "SoftIsp (ONNX) initialized directly";
			return 0;
		} catch (const std::exception &e) {
			LOG(SoftISPPipeline, Error) << "Failed to initialize SoftIsp: " << e.what();
			return 0;
		}
	}
	LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded";
	return 0;
}

void SoftISPCameraData::run()
{
	while (running_) {
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}

void SoftISPCameraData::processRequest(Request *request)
{
	if (!ipa_) {
		this->pipe()->completeRequest(request);
		return;
	}

	static uint32_t frameCounter = 0;
	uint32_t frameId = frameCounter++;

	const auto &buffers = request->buffers();
	if (buffers.empty()) {
		this->pipe()->completeRequest(request);
		return;
	}

	// ============================================
	// CALLER (Pipeline) - Buffer Setup Phase
	// ============================================
	// Pre-calculate sizes, map buffers, prepare resources
	
	for (const auto &bufferPair : buffers) {
		const Stream *stream = bufferPair.first;
		FrameBuffer *fb = bufferPair.second;

		// 1. Get stream configuration (CALCULATE SIZES HERE)
		const StreamConfiguration &cfg = stream->configuration();
		uint32_t width = cfg.size.width;
		uint32_t height = cfg.size.height;
		uint32_t stride = cfg.stride;
		uint32_t pixelFormat = cfg.pixelFormat;

		// Calculate frame buffer size
		// For SBGGR10: 10 bits per pixel, packed
		// Size = (width * 10 / 8) * height (rounded up)
		uint32_t frameSize = 0;
		if (pixelFormat == formats::SBGGR10.code()) {
			frameSize = ((width * 10 + 7) / 8) * height;
		} else if (pixelFormat == formats::NV12.code()) {
			frameSize = width * height * 3 / 2; // Y + UV
		} else {
			frameSize = width * height * 4; // Default to 4 bytes/pixel
		}

		// Calculate stats buffer size (if needed for Stage 1)
		// Stats are typically quarter resolution or fixed size
		uint32_t statsSize = (width / 4) * (height / 4) * sizeof(uint32_t);

		LOG(SoftISPPipeline, Debug) << "Buffer setup: " << width << "x" << height
		                           << ", frameSize=" << frameSize
		                           << ", statsSize=" << statsSize;

		// 2. Map buffers (PREPARE RESOURCES HERE)
		void *frameData = nullptr;
		void *statsData = nullptr;

		if (fb->planes().size() > 0) {
			FrameBuffer::Plane &plane = fb->planes()[0];
			
			// Map frame buffer for read/write access
			frameData = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE,
					 MAP_SHARED, plane.fd.get(), 0);
			
			if (frameData == MAP_FAILED) {
				LOG(SoftISPPipeline, Error) << "Failed to mmap frame buffer";
				frameData = nullptr;
				continue;
			}

			LOG(SoftISPPipeline, Debug) << "Mapped frame buffer at " << frameData
			                           << " (size=" << plane.length << ")";
		}

		// TODO: Map stats buffer if separate
		// if (statsFd valid) {
		//     statsData = mmap(...);
		// }

		// ============================================
		// CALLER PREPARATION COMPLETE
		// Now call CALLEE (IPA) which is stateless
		// ============================================

		// 3. Stage 1: Process Stats (if stats available)
		// IPA reads from pre-mapped statsData, emits metadata
		if (statsData) {
			ipa_->processStats(frameId, 0, ControlList());
		}

		// 4. Stage 2: Process Frame
		// IPA reads Bayer from frameData, applies AWB/AE, writes RGB/YUV back
		// The IPA is STATELESS - no internal state between calls
		ipa_->processFrame(frameId, 0, 
		                   fb->planes()[0].fd, 0, 
		                   width, height, 
		                   ControlList());

		// ============================================
		// CALLER (Pipeline) - Cleanup Phase
		// ============================================
		// Unmap buffers, release resources

		if (frameData) {
			munmap(frameData, frameSize);
			LOG(SoftISPPipeline, Debug) << "Unmapped frame buffer";
		}

		// if (statsData) munmap(...);
	}

	// Complete the request
	this->pipe()->completeRequest(request);
}

// ... (rest of the implementation)

} /* namespace libcamera */
