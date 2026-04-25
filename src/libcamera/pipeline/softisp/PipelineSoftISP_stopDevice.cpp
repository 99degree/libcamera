/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

void PipelineHandlerSoftISP::stopDevice(Camera *camera)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return;
    }
    
    data->stop();
}

} /* namespace libcamera */
