/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

std::unique_ptr<CameraConfiguration> PipelineHandlerSoftISP::generateConfiguration(
	Camera *camera, Span<const StreamRole> roles)
{
	std::cerr << "DEBUG PipelineHandler::generateConfiguration called" << std::endl;
	std::cerr << "DEBUG camera=" << camera << std::endl;
	
	if (!camera) {
		std::cerr << "DEBUG camera is NULL" << std::endl;
		return nullptr;
	}
	
	SoftISPCameraData *data = cameraData(camera);
	std::cerr << "DEBUG data=" << data << std::endl;
	
	if (!data) {
		std::cerr << "DEBUG data is NULL" << std::endl;
		return nullptr;
	}
	
	std::cerr << "DEBUG delegating to data->generateConfiguration()" << std::endl;
	return data->generateConfiguration(roles);
}

} /* namespace libcamera */
