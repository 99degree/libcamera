/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftISP Pipeline Handler - Skeleton Implementation
 */

// Step 1: Include the real libcamera log header
#include <libcamera/base/log.h>
#include <libcamera/internal/ipa_manager.h>

// Step 2: Define the log category for SoftISP pipeline
namespace libcamera {
LOG_DEFINE_CATEGORY(SoftISPPipeline)
} /* namespace libcamera */

// Step 3: Include softisp.h to get all class definitions
#include "softisp.h"

// Step 4: Open namespace for all method implementations
namespace libcamera {

// Step 5: Include all method implementation files
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
static PipelineHandlerFactory<PipelineHandlerSoftISP>
	global_PipelineHandlerSoftISPFactory("softisp");

// Additional Camera methods
#include "SoftISPCamera_frameDone.cpp"
#include "SoftISPCamera_metadataReady.cpp"
#include "SoftISPCamera_tryCompleteRequest.cpp"
#include "SoftISPFrames_destroy.cpp"
#include "SoftISPFrames_find.cpp"
#include "SoftISPCamera_start.cpp"
#include "SoftISPCamera_stop.cpp"
#include "SoftISPCamera_exportFrameBuffers.cpp"
#include "SoftISPCamera_configure.cpp"

} /* namespace libcamera */
