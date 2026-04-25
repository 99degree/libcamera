/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftISP Pipeline Handler - Skeleton Implementation
 */

// Step 1: Include the real libcamera log header FIRST
#include <libcamera/base/log.h>

// Step 2: Define the log category for SoftISP pipeline
namespace libcamera {
LOG_DEFINE_CATEGORY(SoftISPPipeline)
}

// Step 3: Override LOG macro to always use SoftISPPipeline category
// This will be used by all included method files
#undef LOG
#define LOG(category, level, ...) \
    ::libcamera::LOG(SoftISPPipeline, level, __VA_ARGS__)

// Step 4: Include all method implementation files
#include "SoftISPCamera_init.cpp"
#include "SoftISPCamera_loadIPA.cpp"
#include "SoftISPCamera_start.cpp"
#include "SoftISPCamera_stop.cpp"
#include "SoftISPCamera_queueRequest.cpp"
#include "SoftISPCamera_processRequest.cpp"
#include "SoftISPCamera_data.cpp"
#include "SoftISPCamera_configure.cpp"
#include "SoftISPCamera_exportFrameBuffers.cpp"
#include "SoftISPCamera_generateConfiguration.cpp"
#include "SoftISPCamera_getBufferFromId.cpp"
#include "SoftISPCamera_storeBuffer.cpp"
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

// Include the header last (after LOG is defined)
#include "softisp.h"

