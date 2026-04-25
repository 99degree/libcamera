/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

SoftISPCameraData *PipelineHandlerSoftISP::cameraData([[maybe_unused]] Camera *camera)
{
    return virtualCameraData_.get();
}

} /* namespace libcamera */
