int SoftISPCameraData::init()
{
    LOG(SoftISPPipeline, Info) << "Initializing SoftISPCameraData";
    
    // Load IPA module (placeholder for now)
    int ret = loadIPA();
    if (ret < 0)
        return ret;
    
    // Initialize virtual camera
    if (isVirtualCamera) {
        ret = virtualCamera_->init(1920, 1080);
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize virtual camera";
            return ret;
        }
        
        // Start the virtual camera to begin frame generation
        ret = virtualCamera_->start();
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to start virtual camera";
            return ret;
        }
        
        LOG(SoftISPPipeline, Info) << "Virtual camera started and generating frames";
    }
    
    LOG(SoftISPPipeline, Info) << "SoftISPCameraData initialized";
    return 0;
}
