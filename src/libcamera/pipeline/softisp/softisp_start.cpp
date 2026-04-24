int PipelineHandlerSoftISP::start(Camera *camera, const ControlList *controls)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }
    
    return data->start(controls);
}
