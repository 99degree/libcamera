std::unique_ptr<CameraConfiguration> SoftISPCameraData::generateConfiguration(Span<const StreamRole> roles)
{
    LOG(SoftISPPipeline, Info) << "SoftISPCameraData::generateConfiguration called";
    
    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return nullptr;
    }
    
    auto config = std::make_unique<SoftISPConfiguration>();
    if (roles.empty()) {
        return config;
    }
    
    for (const auto& role : roles) {
        switch (role) {
            case StreamRole::StillCapture:
            case StreamRole::VideoRecording:
            case StreamRole::Viewfinder:
                break;
            case StreamRole::Raw:
            default:
                LOG(SoftISPPipeline, Error) << "Unsupported stream role: " << role;
                return nullptr;
        }
        
        unsigned int width = virtualCamera_->width();
        unsigned int height = virtualCamera_->height();
        unsigned int bufferCount = virtualCamera_->bufferCount();
        
        std::map<PixelFormat, std::vector<SizeRange>> streamFormats;
        PixelFormat pixelFormat = formats::SBGGR10;
        streamFormats[pixelFormat] = { SizeRange(Size(width, height), Size(width, height)) };
        
        StreamFormats formats(streamFormats);
        auto cfg = StreamConfiguration(formats);
        cfg.pixelFormat = pixelFormat;
        cfg.size = Size(width, height);
        cfg.bufferCount = bufferCount;
        cfg.colorSpace = ColorSpace::Rec709;
        
        config->addConfiguration(cfg);
        LOG(SoftISPPipeline, Info) << "Added stream: " << width << "x" << height;
    }
    
    if (config->validate() == CameraConfiguration::Invalid) {
        LOG(SoftISPPipeline, Error) << "Invalid configuration";
        return nullptr;
    }
    
    return config;
}
