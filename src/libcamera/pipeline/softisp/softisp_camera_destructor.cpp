SoftISPCameraData::~SoftISPCameraData()
{
    LOG(SoftISPPipeline, Info) << "SoftISPCameraData destroyed";
    Thread::exit(0);
    wait();
}
