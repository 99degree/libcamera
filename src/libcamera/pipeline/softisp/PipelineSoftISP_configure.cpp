/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

int PipelineHandlerSoftISP::configure(Camera *camera, CameraConfiguration *config)
{
    SoftISPCameraData *data = cameraData(camera);
    if (!data) {
        LOG(SoftISPPipeline, Error) << "Failed to get camera data";
        return -EINVAL;
    }
    
    return data->configure(config);
}

} /* namespace libcamera */
