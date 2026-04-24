bool PipelineHandlerSoftISP::match([[maybe_unused]] DeviceEnumerator *enumerator)
{
    // First call: Create and register the virtual camera
    if (!s_virtualCameraRegistered) {
        LOG(SoftISPPipeline, Info) << "Registering virtual camera (first match call)";
        
        // Create camera data
        auto data = std::make_unique<SoftISPCameraData>(this);
        int ret = data->init();
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize camera data";
            return false;
        }
        
        // Store reference to prevent destruction
        virtualCameraData_ = std::move(data);
        
        // Create the camera with empty streams (will be configured later)
        std::set<Stream *> streams;
        std::string id = "softisp_virtual";
        std::shared_ptr<Camera> camera = Camera::create(std::move(virtualCameraData_), id, streams);
        if (!camera) {
            LOG(SoftISPPipeline, Error) << "Failed to create camera";
            return false;
        }
        
        // Register the camera with the CameraManager
        registerCamera(std::move(camera));
        
        // Mark as registered
        s_virtualCameraRegistered = true;
        created_ = true;
        resetCreated_ = true;
        
        LOG(SoftISPPipeline, Info) << "Virtual camera registered successfully";
        return true;
    }
    
    // Subsequent calls: Return false to prevent re-registration
    LOG(SoftISPPipeline, Debug) << "Virtual camera already registered";
    return false;
}
