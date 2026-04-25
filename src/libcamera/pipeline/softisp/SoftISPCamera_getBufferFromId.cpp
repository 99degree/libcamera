/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

FrameBuffer *SoftISPCameraData::getBufferFromId(uint32_t bufferId)
{
    auto it = bufferMap_.find(bufferId);
    return (it != bufferMap_.end()) ? it->second : nullptr;
}

} /* namespace libcamera */
