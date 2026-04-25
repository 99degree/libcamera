/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

int SoftISPCameraData::start([[maybe_unused]] const ControlList *controls)
{
    LOG(SoftISPPipeline, Info) << "Starting camera";
    
    // Start all VirtualCamera instances
    for (auto& [key, virtCam] : virtualCameras_) {
        int ret = virtCam->start();
        if (ret < 0) {
            LOG(SoftISPPipeline, Error) << "Failed to start VirtualCamera: " << key;
            return ret;
        }
        LOG(SoftISPPipeline, Info) << "VirtualCamera started: " << key;
    }
    
    return 0;
}

} /* namespace libcamera */
