/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
    : Camera::Private(pipe)
{
    LOG(SoftISPPipeline, Info) << "SoftISPCameraData created";
}

} /* namespace libcamera */
