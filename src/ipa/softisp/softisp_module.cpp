/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include <libcamera/ipa/ipa_module_info.h>
#include <libcamera/ipa/ipa_interface.h>

// Forward declaration
namespace libcamera { namespace ipa { namespace soft { class SoftIsp; } } }

extern "C" {

const struct IPAModuleInfo ipaModuleInfo = {
    1,
    0,
    "softisp",
    "softisp",
};

libcamera::IPAInterface *ipaCreate()
{
    return new libcamera::ipa::soft::SoftIsp();
}

}
