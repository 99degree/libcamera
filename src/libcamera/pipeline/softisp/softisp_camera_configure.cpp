int SoftISPCameraData::configure(CameraConfiguration *config)
{
    LOG(SoftISPPipeline, Info) << "Configuring camera with " << config->size() << " streams";
    // Configuration is already validated in generateConfiguration
    return 0;
}
