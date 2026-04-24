int SoftISPCameraData::configure(CameraConfiguration *config)
{
    LOG(SoftISPPipeline, Info) << "Configuring camera with " << config->size() << " streams";
    
    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }
    
    // Validate and apply configuration
    if (config->validate() == CameraConfiguration::Invalid) {
        LOG(SoftISPPipeline, Error) << "Invalid camera configuration";
        return -EINVAL;
    }
    
    // Extract stream parameters from configuration
    for (unsigned int i = 0; i < config->size(); ++i) {
        StreamConfiguration &cfg = config->at(i);
        const Stream *stream = cfg.stream();
        
        if (!stream) {
            LOG(SoftISPPipeline, Error) << "Stream " << i << " has no stream object";
            return -EINVAL;
        }
        
        LOG(SoftISPPipeline, Info) << "Stream " << i << ": " 
                                   << cfg.size.width << "x" << cfg.size.height
                                   << " " << cfg.pixelFormat.toString();
        
        // In a full implementation, we would:
        // 1. Set the VirtualCamera resolution
        // 2. Configure the pixel format
        // 3. Set buffer count
    }
    
    LOG(SoftISPPipeline, Info) << "Camera configuration applied successfully";
    return 0;
}
