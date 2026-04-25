/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

SoftISPCameraData::~SoftISPCameraData()
{
	LOG(SoftISPPipeline, Info) << "Destroying SoftISPCameraData";
	
	// Stop the VirtualCamera if running
	if (running_ && virtualCamera_) {
		virtualCamera_->stop();
	}
}

} /* namespace libcamera */
