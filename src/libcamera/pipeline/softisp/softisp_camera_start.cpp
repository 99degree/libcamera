int SoftISPCameraData::start(const ControlList *controls)
{
    LOG(SoftISPPipeline, Info) << "Starting camera";
    (void)controls;
    
    // Start the VirtualCamera now that the camera is actually being used
    if (virtualCamera_) {
        int ret = virtualCamera_->start();
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to start virtual camera";
            return ret;
        }
        LOG(SoftISPPipeline, Info) << "Virtual camera started and generating frames";
    }
    
    running_ = true;
    Thread::start();  // Start the thread
    
    return 0;
}
