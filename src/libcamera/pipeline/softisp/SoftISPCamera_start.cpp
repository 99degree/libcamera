/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

int SoftISPCameraData::start([[maybe_unused]] const ControlList *controls)
{
	LOG(SoftISPPipeline, Info) << "Starting camera";
	
	// Start the VirtualCamera (which has its own thread)
	if (virtualCamera_) {
		int ret = virtualCamera_->start();
		if (ret < 0) {
			LOG(SoftISPPipeline, Error) << "Failed to start VirtualCamera";
			return ret;
		}
		LOG(SoftISPPipeline, Info) << "VirtualCamera started";
	}
	
	running_ = true;
	// Note: We don't call Thread::start() here because SoftISPCameraData doesn't have its own thread
	// The VirtualCamera has its own thread
	
	return 0;
}

} /* namespace libcamera */
