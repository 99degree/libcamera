/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * Custom AF Control IDs for SoftISP
 * 
 * These are temporary IDs until proper controls are added to libcamera/controls.h
 * or defined in a .mojom file.
 */
#pragma once

#include <libcamera/control_ids.h>

namespace libcamera {
namespace controls {

// Custom AF Controls (Temporary)
// These should eventually be moved to controls.h or defined in softisp.mojom
constexpr unsigned int CustomAfFocusMin = 0x80010001;
constexpr unsigned int CustomAfFocusMax = 0x80010002;
constexpr unsigned int CustomAfStepCoarse = 0x80010003;
constexpr unsigned int CustomAfStepFine = 0x80010004;
constexpr unsigned int CustomAfPdafGain = 0x80010005;
constexpr unsigned int CustomAfPdafSquelch = 0x80010006;

} // namespace controls
} // namespace libcamera
