int PipelineHandlerSoftISP::queueRequestDevice(Camera *camera, Request *request)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }
    
    return data->queueRequest(request);
}
