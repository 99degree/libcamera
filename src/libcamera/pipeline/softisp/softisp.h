/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2024
 *
 * Pipeline handler for SoftISP
 */
#pragma once

#include <memory>
#include <string>
#include <vector>

#include <libcamera/base/object.h>
#include <libcamera/base/thread.h>

#include "libcamera/internal/pipeline_handler.h"

namespace libcamera {

class SoftISPCameraData;

/*
 * Pipeline handler for SoftISP.
 *
 * This pipeline handler is designed to work with the SoftISP IPA module
 * which uses ONNX models for image processing.
 */
class PipelineHandlerSoftISP : public PipelineHandler
{
public:
	PipelineHandlerSoftISP(CameraManager *manager);
	~PipelineHandlerSoftISP();

	int match(DeviceEnumerator *enumerator) override;

private:
	int createCamera(DeviceEnumerator *enumerator,
			 const std::string &name);

	/* Helper to get camera data from a Camera */
	SoftISPCameraData *cameraData(const Camera *camera);
};

} /* namespace libcamera */
