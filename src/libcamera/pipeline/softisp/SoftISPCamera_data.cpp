SoftISPCameraData *PipelineHandlerSoftISP::cameraData([[maybe_unused]] Camera *camera)
{
    return virtualCameraData_.get();
}
