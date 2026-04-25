/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

bool PipelineHandlerSoftISP::match([[maybe_unused]] DeviceEnumerator *enumerator)
{
    LOG(SoftISPPipeline, Info) << "PipelineHandlerSoftISP::match() called";
    
    if (!s_virtualCameraRegistered) {
        LOG(SoftISPPipeline, Info) << "Creating and registering virtual camera";
        
        auto data = std::make_unique<SoftISPCameraData>(this);
        if (data->init() < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize camera data";
            return false;
        }
        
        virtualCameraData_ = std::move(data);
        
        std::set<Stream *> streams;
        auto camera = Camera::create(std::move(virtualCameraData_), "softisp_virtual", streams);
        if (!camera) {
            LOG(SoftISPPipeline, Error) << "Failed to create camera";
            return false;
        }
        
        // DO NOT start here - start() will be called by the user
        // registerCamera() adds the camera to the CameraManager's list
        registerCamera(std::move(camera));
        s_virtualCameraRegistered = true;
        created_ = true;
        resetCreated_ = true;
        
        LOG(SoftISPPipeline, Info) << "Virtual camera registered (waiting for open()/start())";
        return true;
    }
    
    return false;
}

} /* namespace libcamera */
