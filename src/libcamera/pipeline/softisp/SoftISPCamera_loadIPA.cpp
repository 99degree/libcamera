/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

#include <libcamera/ipa/ipa_manager.h>
#include <libcamera/ipa/soft_ipa_interface.h>

namespace libcamera {

int SoftISPCameraData::loadIPA()
{
    LOG(SoftISPPipeline, Info) << "Loading IPA module";

    if (isVirtualCamera) {
        LOG(SoftISPPipeline, Info) << "Virtual camera mode - IPA loading skipped";
        return 0;
    }

    // Get the IPA manager
    auto ipaManager = IPAManager::getInstance();
    if (!ipaManager) {
        LOG(SoftISPPipeline, Error) << "Failed to get IPA manager";
        return -ENODEV;
    }

    // Load the SoftISP IPA module
    std::unique_ptr<IPAInterface> ipaInterface = ipaManager->load("SoftISP");
    if (!ipaInterface) {
        LOG(SoftISPPipeline, Error) << "Failed to load SoftISP IPA module";
        return -ENODEV;
    }

    // Cast to our specific interface
    libcamera::ipa::soft::IPASoftInterface *softInterface = 
        dynamic_cast<libcamera::ipa::soft::IPASoftInterface*>(ipaInterface.get());
    
    if (!softInterface) {
        LOG(SoftISPPipeline, Error) << "Invalid IPA interface type";
        return -ENODEV;
    }

    // Store the interface (we're taking ownership)
    ipaInterface_ = softInterface;
    ipaInterfaceOwned_ = std::move(ipaInterface);

    // Initialize the IPA
    int ret = softInterface->init();
    if (ret < 0) {
        LOG(SoftISPPipeline, Error) << "IPA init failed: " << ret;
        return ret;
    }

    // Connect to VirtualCamera if available
    if (virtualCamera_) {
        virtualCamera_->setIPAInterface(softInterface);
        LOG(SoftISPPipeline, Info) << "IPA interface connected to VirtualCamera";
    }

    LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded successfully";
    return 0;
}

} /* namespace libcamera */
