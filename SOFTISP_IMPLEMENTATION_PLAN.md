# SoftISP Implementation Plan for libcamera IPA

## Goal
Implement a platform-independent Software ISP (SoftISP) Image Processing Algorithm (IPA) using ONNX Runtime to load and execute two models (algo.onnx and applier.onnx) to create a complete ISP pipeline that processes Bayer statistics and applies ISP coefficients to generate the final image.

## Reference
- Existing ONNX pipeline example: https://github.com/99degree/softisp-python/blob/main/onnx/test_full_pipeline.py
- Preliminary design: SOFTISP_PLAN.md
- Platform-independent AWB NN IPA plan: PLAN.md (for reference on zone generation)

## Current Status

### Phase 1: Preparation and Setup - COMPLETED

#### Task 1: Create Directory Structure
- Created `src/ipa/softisp/` directory for the new IPA implementation.

#### Task 2: Gather Dependencies
- ONNX Runtime development package dependency added to meson.build
- libcamera IPA development headers are available

#### Task 3: Create Header File
- File: `src/ipa/softisp/softisp.h` - COMPLETED
- Contents:
  - Included `<libcamera/ipa/algorithm.h>`
  - Included ONNX Runtime C++ API: `<onnxruntime/core/session/onnxruntime_cxx_api.h>`
  - Defined class `SoftIsp` : public `libcamera::ipa::Algorithm`
  - Private members:
    * `Ort::Env env_`
    * `std::unique_ptr<Ort::Session> algo_session_`
    * `std::unique_ptr<Ort::Session> applier_session_`
    * `Ort::MemoryInfo memory_info_`
    * Input/output node names (strings) for both models (queried from models)
    * Zone generation parameters (cellsPerZoneX_, cellsPerZoneY_, cellsPerZoneThreshold_)
    * Model file paths (configured via environment variable)
  - Public methods:
    * Constructor and destructor
    * `configure` override
    * `prepare` override
    * `process` override
    * Coefficient getter/setter methods:
      * `setAwbGains()` / `getAwbGains()` for AWB gain updates
      * `setColorTemperature()` / `getColorTemperature()` for color temperature updates
      * `setCcm()` / `getCcm()` for CCM matrix updates

#### Task 4: Create Source File
- File: `src/ipa/softisp/softisp.cpp` - PARTIALLY COMPLETED (stubbed)
- Contents:
  - Constructor: Initialize ONNX runtime environment, load model files.
  - `configure`: 
    * Set up zone generation parameters (similar to Awb algorithm)
    * TODO: Parse configuration for model paths (if provided)
    * TODO: Query model input/output shapes and types for validation
  - `process`:
    * Step 1: Generate zones - NOT YET IMPLEMENTED (stubbed)
    * Step 2: Prepare input tensor for algo.onnx - NOT YET IMPLEMENTED
    * Step 3: Run algo.onnx inference - NOT YET IMPLEMENTED
    * Step 4: Prepare input tensor for applier.onnx from algo.onnx output - NOT YET IMPLEMENTED
    * Step 5: Run applier.onnx inference - NOT YET IMPLEMENTED
    * Step 6: Extract gains tensor - NOT YET IMPLEMENTED
    * Step 7: Assign gains to context.activeState.awb.gains - STUBBED (default values)
    * Step 8: Compute color temperature - NOT YET IMPLEMENTED
    * Step 9: Set temperature in context.activeState.awb.temperatureK - STUBBED (default value)
    * Step 10: Update metadata with colour gains and colour temperature - STUBBED

### Phase 2: Build System Integration - COMPLETED

#### Task 5: Update meson.build
- Added `src/ipa/softisp/softisp.cpp` to the build via `src/ipa/softisp/meson.build`
- Added include directory: `src/ipa/softisp`
- Linked against ONNX Runtime library: `dependencies: [onnxruntime]`
- Ensured C++11 or higher is enabled (via project settings)

#### Task 6: Install Headers
- Installed `softisp.h` to the appropriate include directory via `src/ipa/softisp/meson.build`

### Phase 3: Configuration and Deployment - PARTIALLY COMPLETED

#### Task 7: Make Model Paths Configurable
- Used environment variable (e.g., `SOFTISP_MODEL_DIR`) to specify directory containing algo.onnx and applier.onnx.
- Fallback to `/usr/share/libcamera/ipa/softisp/` if not set.

#### Task 8: Error Handling and Fallback
- Basic error handling for model loading in constructor (catches exceptions and logs error)
- TODO: Implement fallback to grey world AWB when models fail to load or inference fails
- TODO: Implement error handling for inference failures

#### Task 9: Logging
- Added libcamera logging framework with dedicated log category for softisp (`LOG_DEFINE_CATEGORY(SoftIsp)`)

### Phase 4: Testing - NOT STARTED

#### Task 10: Unit Tests
- Not yet created

#### Task 11: Integration Test
- Not yet tested with actual ONNX models (algo.onnx and applier.onnx)

#### Task 12: Performance Considerations
- Not yet profiled

### Phase 5: Documentation - PARTIALLY COMPLETED

#### Task 13: Update Documentation
- Updated SOFTISP_PLAN.md to reflect current implementation status
- Updated PLAN.md to document the SoftISP-based approach
- This file updated to reflect current status

## Files Created/Modified

1. `src/ipa/softisp/softisp.h` (new) - COMPLETED
2. `src/ipa/softisp/softisp.cpp` (new) - PARTIALLY COMPLETED
3. `src/ipa/softisp/meson.build` (new) - COMPLETED
4. `src/ipa/meson.build` (modified to enable softisp build) - COMPLETED
5. `SOFTISP_PLAN.md` (updated) - COMPLETED
6. `PLAN.md` (updated) - COMPLETED
7. `SOFTISP_IMPLEMENTATION_PLAN.md` (this file) - UPDATED

## References and Resources

- ONNX Runtime C++ API: https://onnxruntime.ai/docs/api/cpp/
- libcamera IPA design: https://libcamera.org/
- Existing test_full_pipeline.py for ONNX inference pattern
- SOFTISP_PLAN.md for initial design thoughts
- PLAN.md for platform-independent AWB NN IPA reference (TFLite)

## Open Questions

1. Should we inherit from `Awb` to reuse zone generation code, or reimplement zone generation?
   - We have chosen to reimplement zone generation (as stated in SOFTISP_PLAN.md) to avoid tight coupling with IPU3-specific Awb class.
   - We have duplicated the zone generation logic from Awb algorithm in our implementation plan.

2. How to handle the green gain? The existing Awb algorithm sets green gain to 1.0. Our model may output a different value.
   - We will use the model output for all three gains (as per SOFTISP_PLAN.md) and note that the ISP should handle it.

3. Where to place the model files?
   - We let the user specify via environment variable `SOFTISP_MODEL_DIR`. Default to `/usr/share/libcamera/ipa/softisp/`.

4. Threading: Should we run inference on a separate thread to avoid blocking the IPA pipeline?
   - For simplicity, we run synchronously in the `process` method initially. If performance becomes an issue, we can revisit.

## Next Steps

1. Complete the implementation of the zone generation in the process() method by replicating Awb::process logic up to zone generation.
2. Implement the ONNX inference chain (algo.onnx -> applier.onnx) including tensor preparation and execution.
3. Implement the color temperature computation from the zones using grey world algorithm steps.
4. Set the gains and metadata correctly in the context and ControlList.
5. Add error handling and fallback to grey world AWB when models fail to load or inference fails.
6. Test with actual ONNX models.
