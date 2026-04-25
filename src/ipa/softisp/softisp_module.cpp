/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <libcamera/ipa/ipa_module_info.h>
#include <libcamera/ipa/ipa_interface.h>

// Forward declaration of SoftIsp
namespace libcamera {
namespace ipa {
namespace soft {
class SoftIsp;
}
}
}

extern "C" {

// Module metadata - matches the declaration in ipa_module_info.h
const struct IPAModuleInfo ipaModuleInfo = {
    1,                      // moduleAPIVersion
    0,                      // pipelineVersion
    "softisp",              // pipelineName
    "softisp",              // name (matches ipa_softisp.so -> softisp)
};

// Factory function
libcamera::IPAInterface *ipaCreate()
{
    return new libcamera::ipa::soft::SoftIsp();
}

} /* extern "C" */
