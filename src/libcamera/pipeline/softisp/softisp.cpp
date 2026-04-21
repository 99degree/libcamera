/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP
 */

#include "softisp.h"

#include <algorithm>
#include <cstring>
#include <memory>

#include <libcamera/base/log.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/media_device.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPPipeline)

/*
 * SoftISPCameraData - Camera data structure for SoftISP pipeline.
 *
 * This structure holds all the data needed to manage a camera instance
 * in the SoftISP pipeline.
 */
class SoftISPCameraData
{
public:
	SoftISPCameraData(PipelineHandlerSoftISP *pipe) : pipe_(pipe) {}

	PipelineHandlerSoftISP *pipe_;
};

/* -----------------------------------------------------------------------------
 * PipelineHandlerSoftISP Implementation
 * ---------------------------------------------------------------------------*/

PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager)
	: PipelineHandler(manager)
{
}

PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
}

int PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
	/*
	 * For now, we don't automatically match any devices.
	 * The SoftISP pipeline is intended to be used with specific cameras
	 * configured explicitly, or with the virtual camera for testing.
	 *
	 * In a full implementation, this would enumerate devices and create
	 * camera instances for supported sensors.
	 */
	LOG(SoftISPPipeline, Debug) << "SoftISP pipeline handler initialized";
	return 0;
}

SoftISPCameraData *PipelineHandlerSoftISP::cameraData(const Camera *camera)
{
	return static_cast<SoftISPCameraData *>(camera->privateData());
}

int PipelineHandlerSoftISP::createCamera(DeviceEnumerator *enumerator,
					 const std::string &name)
{
	/*
	 * Create a new camera instance.
	 *
	 * In a full implementation, this would:
	 * 1. Create a MediaDevice for the camera
	 * 2. Enumerate subdevices and video nodes
	 * 3. Create a CameraConfiguration
	 * 4. Create and register the Camera object
	 * 5. Set up the SoftISP IPA module
	 */

	LOG(SoftISPPipeline, Info) << "Creating camera: " << name;

	return 0;
}

} /* namespace libcamera */

/* Register the pipeline handler */
REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "softisp")
