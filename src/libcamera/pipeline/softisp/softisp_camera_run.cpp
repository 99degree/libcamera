void SoftISPCameraData::run()
{
    LOG(SoftISPPipeline, Info) << "SoftISPCameraData thread started";
    
    while (running_) {
        // Thread loop - could be used for background processing
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    LOG(SoftISPPipeline, Info) << "SoftISPCameraData thread stopped";
}
