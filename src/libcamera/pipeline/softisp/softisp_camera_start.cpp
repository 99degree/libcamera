int SoftISPCameraData::start(const ControlList *controls)
{
    LOG(SoftISPPipeline, Info) << "Starting camera";
    (void)controls;
    
    running_ = true;
    Thread::start();  // Start the thread
    
    return 0;
}
