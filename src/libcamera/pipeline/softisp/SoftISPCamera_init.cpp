/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

int SoftISPCameraData::init()
{
	LOG(SoftISPPipeline, Info) << "Initializing SoftISPCameraData";
	
	int ret = loadIPA();
	if (ret) {
		LOG(SoftISPPipeline, Error) << "Failed to load IPA";
		return ret;
	}
	
	// Create VirtualCamera
	virtualCamera_ = std::make_unique<VirtualCamera>();
	ret = virtualCamera_->init(1920, 1080);
	if (ret) {
		LOG(SoftISPPipeline, Error) << "Failed to initialize VirtualCamera";
		return ret;
	}
	
	LOG(SoftISPPipeline, Info) << "VirtualCamera initialized (waiting for start())";
	return 0;
}

} /* namespace libcamera */
