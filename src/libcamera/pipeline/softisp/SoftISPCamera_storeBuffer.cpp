void SoftISPCameraData::storeBuffer(uint32_t bufferId, FrameBuffer *buffer)
{
    std::lock_guard<Mutex> lock(mutex_);
    bufferMap_[bufferId] = buffer;
}
