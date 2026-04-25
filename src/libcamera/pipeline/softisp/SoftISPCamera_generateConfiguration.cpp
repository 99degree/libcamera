/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <libcamera/formats.h>
namespace libcamera {

std::unique_ptr<CameraConfiguration> SoftISPCameraData::generateConfiguration(Span<const StreamRole> roles)
{
    LOG(SoftISPPipeline, Info) << "SoftISPCameraData::generateConfiguration called";
    
    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return nullptr;
    }
    
    auto config = std::make_unique<SoftISPConfiguration>();
    
    // If no roles specified, use default (Viewfinder)
    if (roles.empty()) {
        LOG(SoftISPPipeline, Info) << "No roles specified, using default Viewfinder";
    }
    
    for (const auto& role : roles) {
        LOG(SoftISPPipeline, Info) << "Processing role: " << role;
        
        switch (role) {
            case StreamRole::StillCapture:
            case StreamRole::VideoRecording:
            case StreamRole::Viewfinder:
            case StreamRole::Raw:
                break;
            default:
                LOG(SoftISPPipeline, Error) << "Unsupported stream role: " << role;
                return nullptr;
        }
    }
    
    // Use default configuration if no roles or after processing roles
    unsigned int width = virtualCamera_->width();
    unsigned int height = virtualCamera_->height();
    unsigned int bufferCount = virtualCamera_->bufferCount();
    
    // Create a simple stream configuration
    std::vector<StreamConfiguration> streamConfigs;
    StreamConfiguration cfg;
    cfg.pixelFormat = formats::SBGGR10;
    cfg.size = Size(width, height);
    cfg.bufferCount = bufferCount;
    cfg.colorSpace = ColorSpace::Rec709;
    streamConfigs.push_back(cfg);
    
    // Add configurations to the config object
    for (const auto& scfg : streamConfigs) {
        config->addConfiguration(scfg);
        LOG(SoftISPPipeline, Info) << "Added stream: " << scfg.size.width << "x" << scfg.size.height;
    }
    
    CameraConfiguration::Status status = config->validate();
    if (status == CameraConfiguration::Invalid) {
        LOG(SoftISPPipeline, Error) << "Configuration validation failed with status: " << status;
        return nullptr;
    }
    
    LOG(SoftISPPipeline, Info) << "Configuration generated successfully";
    return config;
}

} /* namespace libcamera */
