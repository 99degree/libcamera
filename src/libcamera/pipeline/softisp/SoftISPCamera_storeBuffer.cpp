/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

void SoftISPCameraData::storeBuffer(uint32_t bufferId, FrameBuffer *buffer)
{
    std::lock_guard<Mutex> lock(mutex_);
    bufferMap_[bufferId] = buffer;
}

} /* namespace libcamera */
