void PipelineHandlerSoftISP::stopDevice(Camera *camera)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return;
    }
    
    data->stop();
}
