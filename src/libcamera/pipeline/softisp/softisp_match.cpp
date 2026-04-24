bool PipelineHandlerSoftISP::match([[maybe_unused]] DeviceEnumerator *enumerator)
{
    LOG(SoftISPPipeline, Info) << "PipelineHandlerSoftISP::match() called";
    
    // TODO: Enumerate hardware cameras (like SimplePipeline does)
    // For now, just register the virtual camera
    
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
        
        // Start the virtual camera immediately
        if (virtualCameraData_->start(nullptr) < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to start virtual camera";
            return false;
        }
        
        registerCamera(std::move(camera));
        s_virtualCameraRegistered = true;
        created_ = true;
        resetCreated_ = true;
        
        LOG(SoftISPPipeline, Info) << "Virtual camera registered successfully";
        LOG(SoftISPPipeline, Info) << "Run 'cam --list' to see the camera";
        return true;
    }
    
    return false;
}
