void SoftISPCameraData::stop()
{
    LOG(SoftISPPipeline, Info) << "Stopping camera";
    
    // Stop all VirtualCamera instances
    for (auto& [key, virtCam] : virtualCameras_) {
        virtCam->stop();
        LOG(SoftISPPipeline, Info) << "VirtualCamera stopped: " << key;
    }
}
