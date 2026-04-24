bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
    LOG(SoftISPPipeline, Info) << "Matching devices...";
    
    // Check if any hardware cameras exist by trying to find a V4L2 device
    bool hasHardwareCamera = false;
    
    // Try to find common V4L2 devices
    const char *commonDrivers[] = { "uvcvideo", "bcm2835-isp", "rkisp", "mmparser", nullptr };
    
    for (int i = 0; commonDrivers[i] != nullptr; i++) {
        DeviceMatch dm(commonDrivers[i]);
        auto device = enumerator->search(dm);
        if (device) {
            hasHardwareCamera = true;
            LOG(SoftISPPipeline, Info) << "Hardware camera found: " << device->name();
            break;
        }
    }
    
    // If no hardware cameras found, register the virtual camera
    if (!hasHardwareCamera && !s_virtualCameraRegistered) {
        LOG(SoftISPPipeline, Info) << "No hardware cameras found, registering virtual camera";
        
        // Create camera data
        auto data = std::make_unique<SoftISPCameraData>(this);
        int ret = data->init();
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to initialize camera data";
            return false;
        }
        
        // Store reference
        virtualCameraData_ = std::move(data);
        
        // Create and register the camera
        std::set<Stream *> streams;
        std::string id = "softisp_virtual";
        std::shared_ptr<Camera> camera = Camera::create(std::move(virtualCameraData_), id, streams);
        if (!camera) {
            LOG(SoftISPPipeline, Error) << "Failed to create camera";
            return false;
        }
        
        registerCamera(std::move(camera));
        
        // Mark as registered
        s_virtualCameraRegistered = true;
        created_ = true;
        resetCreated_ = true;
        
        LOG(SoftISPPipeline, Info) << "Virtual camera registered successfully";
        return true;
    }
    
    if (hasHardwareCamera) {
        LOG(SoftISPPipeline, Info) << "Hardware cameras present, virtual camera not needed";
    } else if (s_virtualCameraRegistered) {
        LOG(SoftISPPipeline, Debug) << "Virtual camera already registered";
    }
    
    return false;
}
