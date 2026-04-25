/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - Standalone stub IPA implementation.
 * Provides logging and basic structure without dependencies.
 */
#include <libcamera/base/log.h>
#include <libcamera/ipa/ipa_module_info.h>
#include <libcamera/ipa/ipa_interface.h>
#include <libcamera/controls.h>
#include <libcamera/base/shared_fd.h>
#include <memory>

namespace libcamera {
namespace ipa::soft {

// Simple stub class that inherits from IPAInterface
class SoftIspStub : public IPAInterface
{
public:
	SoftIspStub() {
		LOG(SoftIsp, Info) << "SoftIspStub created";
	}
	
	~SoftIspStub() override {
		LOG(SoftIsp, Info) << "SoftIspStub destroyed";
	}
};

} /* namespace soft */

/* External IPA module interface */
extern "C" {

const struct IPAModuleInfo ipaModuleInfo = {
	IPA_MODULE_API_VERSION,
	0,
	"softisp",
	"SoftISP Stub",
};

IPAInterface *ipaCreate()
{
	LOG(SoftIsp, Info) << "Creating SoftISP stub IPA module";
	return new ipa::soft::SoftIspStub();
}

} /* extern "C" */

} /* namespace libcamera */
