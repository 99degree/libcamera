FrameBuffer *SoftISPCameraData::getBufferFromId(uint32_t bufferId)
{
    auto it = bufferMap_.find(bufferId);
    return (it != bufferMap_.end()) ? it->second : nullptr;
}
