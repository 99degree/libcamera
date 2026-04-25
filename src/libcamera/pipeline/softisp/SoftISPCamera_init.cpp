/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

int SoftISPCameraData::init()
{
    LOG(SoftISPPipeline, Info) << "Initializing SoftISPCameraData";
    
    // Create VirtualCamera instance(s) and store in map
    // For now, create one virtual camera with default resolution
    auto virtCam = std::make_unique<VirtualCamera>();
    int ret = virtCam->init(1920, 1080);
    if (ret < 0) {
        LOG(SoftISPPipeline, Error) << "Failed to initialize VirtualCamera";
        return ret;
    }
    
    // Store in map with a unique key
    virtualCameras_["default"] = std::move(virtCam);
    
    LOG(SoftISPPipeline, Info) << "Virtual camera initialized (waiting for start())";
    LOG(SoftISPPipeline, Info) << "SoftISPCameraData initialized";
    
    return 0;
}

} /* namespace libcamera */
