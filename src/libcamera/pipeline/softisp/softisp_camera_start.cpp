int SoftISPCameraData::start([[maybe_unused]] const ControlList *controls)
{
    LOG(SoftISPPipeline, Info) << "Starting camera";
    
    if (!virtualCamera_) {
        LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
        return -EINVAL;
    }
    
    // Start the VirtualCamera - it manages its own thread and frame generation
    return virtualCamera_->start();
}
