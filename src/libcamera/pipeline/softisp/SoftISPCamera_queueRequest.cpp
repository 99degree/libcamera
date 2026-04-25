/* SPDX-License-Identifier: LGPL-2.1-or-later */
namespace libcamera {

int SoftISPCameraData::queueRequest(Request *request)
{
    LOG(SoftISPPipeline, Info) << "Queueing request";
    
    if (!request) {
        LOG(SoftISPPipeline, Error) << "Null request";
        return -EINVAL;
    }
    
    // Process the request
    processRequest(request);
    
    return 0;
}

} /* namespace libcamera */
