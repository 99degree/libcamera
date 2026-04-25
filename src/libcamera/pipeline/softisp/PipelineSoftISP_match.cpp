/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include "placeholder_stream.h"
#include <libcamera/formats.h>
#include "libcamera/internal/device_enumerator.h"
#include <algorithm>
#include <set>

namespace libcamera {

// Static camera that persists across pipeline handler lifetimes
static std::shared_ptr<Camera> s_persistentCamera;

bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
    (void)enumerator;
    
    std::cerr << "DEBUG match() called" << std::endl;
    
    // If we already have a persistent camera, just return true
    if (s_persistentCamera) {
        std::cerr << "DEBUG match(): using existing persistent camera" << std::endl;
        registerCamera(s_persistentCamera);
        return true;
    }
    
    std::cerr << "DEBUG match(): creating new camera data" << std::endl;
    auto data = std::make_unique<SoftISPCameraData>(this);
    std::cerr << "DEBUG match(): data=" << data.get() << std::endl;
    
    if (data->init() < 0) {
        std::cerr << "DEBUG match(): init failed" << std::endl;
        return false;
    }
    
    // Create a placeholder stream
    data->initialStream_ = new PlaceholderStream();
    std::set<Stream *> streams;
    streams.insert(data->initialStream_);
    
    std::cerr << "DEBUG match(): calling Camera::create()" << std::endl;
    auto camera = Camera::create(std::move(data), "softisp_virtual", streams);
    if (!camera) {
        std::cerr << "DEBUG match(): Camera::create() returned nullptr" << std::endl;
        return false;
    }
    
    // Store the camera statically so it persists
    s_persistentCamera = camera;
    
    std::cerr << "DEBUG match(): calling registerCamera()" << std::endl;
    registerCamera(std::move(camera));
    
    std::cerr << "DEBUG match(): returning true" << std::endl;
    return true;
}

} /* namespace libcamera */
