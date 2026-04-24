bool PipelineHandlerSoftISP::match([[maybe_unused]] DeviceEnumerator *enumerator)
{
    LOG(SoftISPPipeline, Info) << "Matching devices...";
    
    // Register virtual camera only once
    if (!s_virtualCameraRegistered) {
        LOG(SoftISPPipeline, Info) << "Registering virtual camera";
        
        auto data = std::make_unique<SoftISPCameraData>(this);
        if (data->init() < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize";
            return false;
        }
        
        virtualCameraData_ = std::move(data);
        
        std::set<Stream *> streams;
        auto camera = Camera::create(std::move(virtualCameraData_), "softisp_virtual", streams);
        if (!camera) {
            LOG(SoftISPPipeline, Error) << "Failed to create camera";
            return false;
        }
        
        registerCamera(std::move(camera));
        s_virtualCameraRegistered = true;
        created_ = true;
        resetCreated_ = true;
        
        LOG(SoftISPPipeline, Info) << "Virtual camera registered";
        return true;
    }
    
    return false;
}
