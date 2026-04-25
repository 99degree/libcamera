/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftISP Pipeline Handler - Skeleton Implementation
 */

// Step 1: Include the real libcamera log header
#include <libcamera/base/log.h>

// Step 2: Define the log category for SoftISP pipeline
namespace libcamera {
LOG_DEFINE_CATEGORY(SoftISPPipeline)

// Step 3: Define a static const reference to the category for easy use with LOG macro
static const LogCategory &SoftISPPipeline = _LOG_CATEGORY(SoftISPPipeline)();

} /* namespace libcamera */

// Step 4: Include softisp.h to get all class definitions
#include "softisp.h"

// Step 5: Include all method implementation files
// Each included file has its own namespace libcamera { } block
#include "PipelineSoftISP_constructor.cpp"
#include "PipelineSoftISP_destructor.cpp"
#include "PipelineSoftISP_match.cpp"
#include "PipelineSoftISP_configure.cpp"
#include "PipelineSoftISP_start.cpp"
#include "PipelineSoftISP_stopDevice.cpp"
#include "PipelineSoftISP_queueRequestDevice.cpp"
#include "PipelineSoftISP_exportFrameBuffers.cpp"
#include "PipelineSoftISP_generateConfiguration.cpp"
#include "PipelineSoftISP_new.cpp"

#include "SoftISPCamera_constructor.cpp"
#include "SoftISPCamera_destructor.cpp"
#include "SoftISPCamera_init.cpp"
#include "SoftISPCamera_loadIPA.cpp"
#include "SoftISPCamera_queueRequest.cpp"
#include "SoftISPCamera_generateConfiguration.cpp"
#include "SoftISPCamera_getBufferFromId.cpp"
#include "SoftISPCamera_storeBuffer.cpp"
#include "SoftISPCamera_data.cpp"

#include "SoftISPConfig_constructor.cpp"
#include "SoftISPConfig_validate.cpp"

#include "virtual_camera.cpp"

// Register the pipeline handler manually
namespace libcamera {
static PipelineHandlerFactory<PipelineHandlerSoftISP> global_PipelineHandlerSoftISPFactory("softisp");
} /* namespace libcamera */
#include "SoftISPCamera_frameDone.cpp"

// Additional Camera methods
#include "SoftISPCamera_start.cpp"
#include "SoftISPCamera_stop.cpp"
#include "SoftISPCamera_exportFrameBuffers.cpp"
#include "SoftISPCamera_configure.cpp"

// Initialize static member
