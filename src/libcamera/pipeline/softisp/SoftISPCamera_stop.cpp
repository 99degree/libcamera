/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

void SoftISPCameraData::stop()
{
	LOG(SoftISPPipeline, Info) << "Stopping camera";
	
	// Stop the VirtualCamera
	if (virtualCamera_) {
		virtualCamera_->stop();
		LOG(SoftISPPipeline, Info) << "VirtualCamera stopped";
	}
	
	running_ = false;
	// Note: No Thread::exit() or wait() because SoftISPCameraData doesn't have its own thread
}

} /* namespace libcamera */
