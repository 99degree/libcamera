/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

namespace libcamera {

int SoftISPCameraData::configure(CameraConfiguration *config)
{
	LOG(SoftISPPipeline, Info) << "=== configure() called ===";
	LOG(SoftISPPipeline, Info) << "Configuring camera with " << config->size() << " streams";
	
	if (!virtualCamera_) {
		LOG(SoftISPPipeline, Error) << "VirtualCamera not initialized";
		return -EINVAL;
	}
	
	if (!initialStream_) {
		LOG(SoftISPPipeline, Error) << "initialStream_ is NULL!";
		return -EINVAL;
	}
	
	// Validate first
	CameraConfiguration::Status status = config->validate();
	if (status == CameraConfiguration::Invalid) {
		LOG(SoftISPPipeline, Error) << "Invalid camera configuration";
		return -EINVAL;
	}
	LOG(SoftISPPipeline, Info) << "Configuration validated";
	
	// Set the stream for each configuration - use the SAME stream object
	for (unsigned int i = 0; i < config->size(); ++i) {
		StreamConfiguration &cfg = config->at(i);
		
		LOG(SoftISPPipeline, Info) << "Stream " << i << " initialStream_ address: " << initialStream_;
		
		cfg.setStream(initialStream_);  // Use the same stream!
		
		const Stream *stream = cfg.stream();
		LOG(SoftISPPipeline, Info) << "Stream " << i << " cfg.stream() address: " << stream;
		
		if (!stream) {
			LOG(SoftISPPipeline, Error) << "Stream " << i << " has no stream object!";
			return -EINVAL;
		}
		
		LOG(SoftISPPipeline, Info) << "Stream " << i << ": " << cfg.size.width << "x" << cfg.size.height << " " << cfg.pixelFormat.toString();
	}
	
	LOG(SoftISPPipeline, Info) << "Camera configuration applied successfully";
	return 0;
}

} /* namespace libcamera */
