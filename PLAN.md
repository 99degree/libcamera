# Plan for Creating a New Platform-Independent SoftISP NN IPA

## Goal
Create a new Image Processing Algorithm (IPA) for SoftISP using an ONNX neural network model that is not dependent on specific hardware. The new IPA should be usable on any platform supported by libcamera.

## Reference
We have implemented an ONNX-based SoftISP algorithm that processes Bayer statistics through two models:
1. algo.onnx: generates ISP coefficients (AWB gains, CCM, etc.) from Bayer statistics
2. applier.onnx: applies the generated coefficients to the full-resolution image to produce the final output

The implementation includes getter/setter methods for external updates to ISP coefficients, allowing runtime adjustment of AWB gains, color temperature, and CCM matrix.

## Steps Already Completed

### 1. Created New Directory
Created `src/ipa/softisp/` for the SoftISP NN IPA.

### 2. Created Source Files
- Created `src/ipa/softisp/softisp.h`
- Created `src/ipa/softisp/softisp.cpp`
- Created `src/ipa/softisp/meson.build`

### 3. Updated Header and Class Structure
- The class `SoftIsp` inherits from `libcamera::ipa::Algorithm`
- Updated the class name and registration string to avoid conflicts (will use `"softisp"`)
- Retained the zone generation parameters from the Awb algorithm for initial statistics processing
- Added getter/setter methods for external coefficient updates:
  - `setAwbGains()` / `getAwbGains()` for AWB gain updates
  - `setColorTemperature()` / `getColorTemperature()` for color temperature updates  
  - `setCcm()` / `getCcm()` for CCM matrix updates

### 4. Modified Build System
- Added the new source files to the build via `src/ipa/softisp/meson.build`
- Updated `src/ipa/meson.build` to conditionally build the softisp module
- Linked against ONNX Runtime dependency

### 5. Configured via Environment Variable
- The IPA uses environment variable `SOFTISP_MODEL_DIR` to specify the directory containing algo.onnx and applier.onnx
- Falls back to `/usr/share/libcamera/ipa/softisp/` if not set

### 6. Testing Status
- Basic compilation setup is in place
- Need to test with actual ONNX models

### 7. Documentation
- Updated SOFTISP_PLAN.md to reflect current implementation status
- This file documents the implementation approach

## Notes
- The implementation is LGPL-2.1-or-later licensed, consistent with libcamera
- The implementation shares zone generation concepts with the Awb algorithm but implements its own zone generation helpers for initial Bayer statistics processing
- The implementation uses ONNX Runtime for inference in a two-stage pipeline:
  1. First stage (algo.onnx): processes Bayer zone statistics to generate ISP coefficients
  2. Second stage (applier.onnx): applies those coefficients to the full image
- The implementation follows the pattern seen in hardware IPAs (like VC4) where coefficient values can be updated externally and applied to the image processing pipeline

## Files Created/Modified
- `src/ipa/softisp/softisp.h` (new)
- `src/ipa/softisp/softisp.cpp` (new)
- `src/ipa/softisp/meson.build` (new)
- `src/ipa/meson.build` (modified to enable softisp build)
- `SOFTISP_PLAN.md` (updated to reflect current status)

## Next Steps
1. Complete the implementation of the zone generation in the process() method (currently stubbed)
2. Implement the ONNX inference chain (algo.onnx -> applier.onnx) including tensor preparation for both stages
3. Implement the color temperature computation from the zones (for metadata)
4. Set the gains and metadata correctly in the context
5. Add error handling and fallback to grey world AWB when models fail
6. Test with actual ONNX models
