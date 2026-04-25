/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <libcamera/formats.h>

namespace libcamera {

std::unique_ptr<CameraConfiguration> SoftISPCameraData::generateConfiguration(Span<const StreamRole> roles)
{
	std::cerr << "DEBUG generateConfiguration: roles.size()=" << roles.size() << std::endl;
	
	if (!virtualCamera_) {
		std::cerr << "DEBUG generateConfiguration: virtualCamera_ is NULL" << std::endl;
		return nullptr;
	}
	
	if (!initialStream_) {
		std::cerr << "DEBUG generateConfiguration: initialStream_ is NULL!" << std::endl;
		return nullptr;
	}
	
	std::cerr << "DEBUG generateConfiguration: initialStream_ address: " << initialStream_ << std::endl;
	
	auto config = std::make_unique<SoftISPConfiguration>();
	std::cerr << "DEBUG generateConfiguration: config created" << std::endl;
	
	for (const auto& role : roles) {
		std::cerr << "DEBUG generateConfiguration: role=" << static_cast<int>(role) << std::endl;
		switch (role) {
		case StreamRole::StillCapture:
		case StreamRole::VideoRecording:
		case StreamRole::Viewfinder:
		case StreamRole::Raw:
			break;
		default:
			std::cerr << "DEBUG generateConfiguration: unsupported role" << std::endl;
			return nullptr;
		}
	}
	
	unsigned int width = virtualCamera_->width();
	unsigned int height = virtualCamera_->height();
	unsigned int bufferCount = virtualCamera_->bufferCount();
	
	std::cerr << "DEBUG generateConfiguration: " << width << "x" << height << ", buffers=" << bufferCount << std::endl;
	
	if (width == 0 || height == 0) {
		std::cerr << "DEBUG generateConfiguration: invalid dimensions" << std::endl;
		return nullptr;
	}
	
	// Use the same stream object that was created in match()
	StreamConfiguration cfg;
	cfg.pixelFormat = formats::SBGGR10;
	cfg.size = Size(width, height);
	cfg.bufferCount = bufferCount;
	cfg.colorSpace = ColorSpace::Rec709;
	cfg.setStream(initialStream_);  // Use the same stream!
	
	std::cerr << "DEBUG generateConfiguration: cfg.stream()=" << cfg.stream() << std::endl;
	
	config->addConfiguration(cfg);
	std::cerr << "DEBUG generateConfiguration: added stream, config->size()=" << config->size() << std::endl;
	
	CameraConfiguration::Status status = config->validate();
	std::cerr << "DEBUG generateConfiguration: validate() returned " << static_cast<int>(status) << std::endl;
	
	if (status == CameraConfiguration::Invalid) {
		std::cerr << "DEBUG generateConfiguration: validation failed" << std::endl;
		return nullptr;
	}
	
	std::cerr << "DEBUG generateConfiguration: SUCCESS, returning config with size=" << config->size() << std::endl;
	return config;
}

} /* namespace libcamera */
