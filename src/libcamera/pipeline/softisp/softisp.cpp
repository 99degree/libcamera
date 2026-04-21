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
#include <libcamera/control_ids.h>
#include <libcamera/controls.h>

#include "libcamera/internal/camera.h"
#include "libcamera/internal/camera_manager.h"
#include "libcamera/internal/camera_sensor.h"
#include "libcamera/internal/device_enumerator.h"
#include "libcamera/internal/formats.h"
#include "libcamera/internal/media_device.h"
#include "libcamera/internal/request.h"
#include "libcamera/internal/v4l2_subdevice.h"
#include "libcamera/internal/v4l2_videodevice.h"
#include "libcamera/internal/ipa_manager.h"
#include "libcamera/internal/ipa_module.h"
#include "libcamera/ipa/soft_ipa_interface.h"

namespace libcamera {

LOG_DEFINE_CATEGORY(SoftISPPipeline)

/*
 * SoftISPCameraData - Camera data structure for SoftISP pipeline.
 *
 * This structure holds all the data needed to manage a camera instance
 * in the SoftISP pipeline, including the IPA module interface.
 */
class SoftISPCameraData : public Camera::Private
{
public:
	SoftISPCameraData(PipelineHandlerSoftISP *pipe, MediaEntity *entity);
	~SoftISPCameraData();

	int init();
	int loadIPA();

	/* Placeholder for future implementation */
	void updateControls(const ControlInfoMap &ipaControls);

	std::unique_ptr<CameraSensor> sensor_;
	MediaEntity *entity_;
	std::unique_ptr<V4L2Subdevice> sensorSubdev_;
	std::unique_ptr<V4L2VideoDevice> videoCapture_;
	std::unique_ptr<ipa::soft::IPAProxySoft> ipa_;
	ControlInfoMap controlInfo_;
};

/* -----------------------------------------------------------------------------
 * SoftISPCameraData Implementation
 * ---------------------------------------------------------------------------*/

SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe,
				     MediaEntity *entity)
	: Camera::Private(pipe), entity_(entity)
{
}

SoftISPCameraData::~SoftISPCameraData()
{
}

int SoftISPCameraData::init()
{
	/*
	 * Initialize camera sensor and subdevices.
	 * This would typically enumerate the media graph and create
	 * V4L2Subdevice/V4L2VideoDevice objects for each entity.
	 */
	LOG(SoftISPPipeline, Debug) << "Initializing SoftISP camera";

	/* For now, just return success */
	return 0;
}

int SoftISPCameraData::loadIPA()
{
	/*
	 * Load the SoftISP IPA module.
	 *
	 * The IPAManager will search for an IPA module where:
	 * - pipelineName matches the pipeline handler name ("softisp")
	 * - pipelineVersion is within the specified range (0, 0 = any)
	 *
	 * The matched module's ipaCreate() function will be called to
	 * create the IPA context, which will be wrapped in an
	 * IPAProxySoft object.
	 */
	ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoft>(pipe(), 0, 0);
	if (!ipa_) {
		LOG(SoftISPPipeline, Error)
			<< "Failed to create SoftISP IPA module";
		return -ENOENT;
	}

	LOG(SoftISPPipeline, Info) << "SoftISP IPA module loaded successfully";

	/*
	 * Connect IPA signals to camera data handlers.
	 * These would be implemented when full frame processing is added.
	 */
	/*
	ipa_->setSensorControls.connect(
		this, &SoftISPCameraData::setSensorControls);
	ipa_->metadataReady.connect(
		this, &SoftISPCameraData::metadataReady);
	*/

	/*
	 * Get the configuration file path from the IPA module.
	 * The IPA module can provide a tuning file based on the sensor model.
	 */
	std::string ipaTuningFile;
	if (sensor_) {
		ipaTuningFile = ipa_->configurationFile(
			sensor_->model() + ".yaml", "uncalibrated.yaml");
		LOG(SoftISPPipeline, Debug)
			<< "IPA tuning file: " << ipaTuningFile;
	}

	/*
	 * Get the controls supported by the IPA module.
	 * These would be merged with the sensor controls.
	 */
	/*
	ControlInfoMap ipaControls = ipa_->controls();
	updateControls(ipaControls);
	*/

	return 0;
}

void SoftISPCameraData::updateControls(const ControlInfoMap &ipaControls)
{
	/*
	 * Merge IPA controls with sensor controls.
	 * This would be implemented when full camera control is added.
	 */
	controlInfo_ = ipaControls;
}

/* -----------------------------------------------------------------------------
 * PipelineHandlerSoftISP Implementation
 * ---------------------------------------------------------------------------*/

PipelineHandlerSoftISP::PipelineHandlerSoftISP(CameraManager *manager)
	: PipelineHandler(manager)
{
	LOG(SoftISPPipeline, Info) << "SoftISP pipeline handler initialized";
}

PipelineHandlerSoftISP::~PipelineHandlerSoftISP()
{
}

int PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
	/*
	 * Enumerate devices and create camera instances for supported sensors.
	 *
	 * In a full implementation, this would:
	 * 1. Iterate through all media devices
	 * 2. Check if each device has a supported sensor
	 * 3. Create a SoftISPCameraData instance for each supported camera
	 * 4. Call camera->create() to register the camera with libcamera
	 */

	LOG(SoftISPPipeline, Debug) << "Matching devices for SoftISP pipeline";

	/*
	 * For now, we don't automatically match any devices.
	 * This is a placeholder that can be expanded to support specific
	 * cameras or used with the virtual camera for testing.
	 */

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
	 * Create a new camera instance for the given device.
	 *
	 * This would typically:
	 * 1. Open the media device
	 * 2. Create a MediaDevice object
	 * 3. Enumerate subdevices and video nodes
	 * 4. Create a SoftISPCameraData instance
	 * 5. Call cameraData->init() to initialize the camera
	 * 6. Call cameraData->loadIPA() to load the IPA module
	 * 7. Create and register the Camera object
	 */

	LOG(SoftISPPipeline, Info) << "Creating camera: " << name;

	/* Placeholder implementation */
	return 0;
}

} /* namespace libcamera */

/* Register the pipeline handler */
REGISTER_PIPELINE_HANDLER(PipelineHandlerSoftISP, "softisp")
