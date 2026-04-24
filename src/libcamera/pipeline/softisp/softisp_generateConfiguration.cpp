std::unique_ptr<CameraConfiguration> PipelineHandlerSoftISP::generateConfiguration(
    Camera *camera, Span<const StreamRole> roles)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return nullptr;
    }
    
    return data->generateConfiguration(roles);
}
