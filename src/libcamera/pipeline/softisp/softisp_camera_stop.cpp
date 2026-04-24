void SoftISPCameraData::stop()
{
    LOG(SoftISPPipeline, Info) << "Stopping camera";
    
    running_ = false;
    Thread::exit(0);
    Thread::wait();
}
