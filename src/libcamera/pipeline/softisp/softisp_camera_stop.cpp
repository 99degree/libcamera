void SoftISPCameraData::stop()
{
    LOG(SoftISPPipeline, Info) << "Stopping camera";
    
    if (virtualCamera_) {
        virtualCamera_->stop();
    }
}
