/* SPDX-License-Identifier: LGPL-2.1-or-later */
/* SoftISP IPA Interface - Stub implementation */
#ifndef LIBCAMERA_IPA_SOFT_IPA_INTERFACE_H
#define LIBCAMERA_IPA_SOFT_IPA_INTERFACE_H

#include <libcamera/ipa/ipa_interface.h>
#include <libcamera/ipa/ipa_controls.h>
#include <libcamera/base/shared_fd.h>
#include <libcamera/controls.h>
#include <libcamera/geometry.h>

namespace libcamera {
namespace ipa {
namespace soft {

// Settings structures
struct IPASettings {
	int width = 0;
	int height = 0;
};

struct IPACameraSensorInfo {
	Rectangle activeArea;
	Rectangle pixelArrayArea;
};

struct IPAConfigInfo {
	// Configuration info
};

// Forward declaration of the interface
class IPASoftInterface : public IPAInterface
{
public:
	virtual ~IPASoftInterface() = default;
	
	virtual int32_t init(const IPASettings &settings,
			     const SharedFD &fdStats,
			     const SharedFD &fdParams,
			     const IPACameraSensorInfo &sensorInfo,
			     const ControlInfoMap &sensorControls,
			     ControlInfoMap *ipaControls,
			     bool *ccmEnabled) = 0;
	
	virtual int32_t start() = 0;
	virtual void stop() = 0;
	virtual int32_t configure(const IPAConfigInfo &configInfo) = 0;
	virtual void queueRequest(uint32_t frame, const ControlList &controls) = 0;
	virtual void computeParams(uint32_t frame) = 0;
	virtual void processStats(uint32_t frame,
				  uint32_t bufferId,
				  ControlList &stats) = 0;
};

} /* namespace soft */
} /* namespace ipa */
} /* namespace libcamera */

#endif /* LIBCAMERA_IPA_SOFT_IPA_INTERFACE_H */
