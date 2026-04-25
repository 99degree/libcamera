/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

void SoftISPCameraData::stop()
{
    LOG(SoftISPPipeline, Info) << "Stopping camera";
    
    // Stop all VirtualCamera instances
    for (auto& [key, virtCam] : virtualCameras_) {
        virtCam->stop();
        LOG(SoftISPPipeline, Info) << "VirtualCamera stopped: " << key;
    }
}

} /* namespace libcamera */
