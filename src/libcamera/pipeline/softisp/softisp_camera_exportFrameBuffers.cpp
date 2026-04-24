int SoftISPCameraData::exportFrameBuffers([[maybe_unused]] Stream *stream, [[maybe_unused]] std::vector<std::unique_ptr<FrameBuffer>> *buffers)
{
    LOG(SoftISPPipeline, Info) << "Exporting frame buffers for stream";
    // Frame buffer export would be handled by the pipeline handler
    // or by creating synthetic buffers for the virtual camera
    return -ENOTSUP;
}
