int SoftISPCameraData::loadIPA()
{
    LOG(SoftISPPipeline, Info) << "Loading IPA module (virtual mode)";
    // IPA loading is disabled for virtual camera mode
    // In real hardware mode, this would load the IPA module
    return 0;
}
