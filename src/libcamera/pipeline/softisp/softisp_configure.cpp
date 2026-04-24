int PipelineHandlerSoftISP::configure(Camera *camera, CameraConfiguration *config)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }
    
    return data->configure(config);
}
