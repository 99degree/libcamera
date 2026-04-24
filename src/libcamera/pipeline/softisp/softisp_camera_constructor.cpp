SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
    : Camera::Private(pipe),
      virtualCamera_(std::make_unique<VirtualCamera>())
{
    LOG(SoftISPPipeline, Info) << "SoftISPCameraData created";
}
