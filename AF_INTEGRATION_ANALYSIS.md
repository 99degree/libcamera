# Comprehensive Analysis Report: SoftISP Pipeline, IPA, and AF Integration

## Overview

After analyzing the codebase from April 20-26, 2026, I've identified several critical issues across the three main components: SoftISP pipeline/virtual camera, SoftISP IPA, and AF integration. These issues indicate that while the framework is in place, critical functionality is missing or incomplete.

## Part 1: SoftISP Pipeline/Virtual Camera Issues

### Critical Issues:

1. **Missing processFrame Implementation**: The VirtualCamera class has a stub implementation that references `processWithIPA()` but this method is never actually called in the main processing loop.

2. **Incomplete IPA Integration**: The `processWithIPA()` method exists but is not properly integrated into the frame processing pipeline.

3. **No AF Support**: The virtual camera implementation doesn't include any AF-related functionality.

### Code Analysis:

The virtual camera implementation in `src/libcamera/pipeline/softisp/virtual_camera.cpp` has the following issues:

1. The `processWithIPA()` method is defined but only called once in a debug log statement, not in the actual frame processing.
2. The main `processFrame()` method doesn't call the IPA processing.
3. No AF controls or statistics are generated or processed.

## Part 2: SoftISP IPA Issues

### Critical Issues:

1. **Missing processFrame Implementation**: The `processFrame()` method is declared in the header but never implemented.

2. **Incomplete Architecture**: Documentation mentions a dual-stage processing (processStats for AWB/AE and processFrame for Bayer → RGB/YUV) but only processStats is implemented.

3. **No AF Integration**: The IPA module doesn't include any AF-related processing.

### Code Analysis:

1. The `processFrame()` method signature exists in `softisp.h` but there's no implementation file.
2. Only `processStats()` is implemented in `SoftIsp_processStats.cpp`.
3. The ONNX engine is set up for dual models (algoEngine and applierEngine) but only one seems to be used.

## Part 3: AF Integration Issues

### Critical Issues:

1. **Uninitialized AF Variables**: The AF statistics accumulation variables `afGradientSum_` and `afPixelCount_` are declared but never initialized.

2. **Missing AF Statistics Conversion**: The accumulated AF gradient values are never converted to the final AF statistics that should be stored in the `SwIspStats` structure.

3. **Incomplete AF Integration**: The `finishFrame` method doesn't set the AF statistics in the shared memory structure.

### Code Analysis:

1. In `src/libcamera/software_isp/swstats_cpu.h`, the member variables `afGradientSum_` and `afPixelCount_` are declared but not initialized in the constructor.

2. In `src/libcamera/software_isp/swstats_cpu.cpp`, the AF gradient values are accumulated in various line processing functions but never converted to final statistics.

3. The `finishFrame` method in `swstats_cpu.cpp` doesn't set the AF statistics fields (`afContrast`, `afPhaseError`, etc.) in the shared `SwIspStats` structure.

4. The `SwIspStats` structure in `include/libcamera/internal/software_isp/swisp_stats.h` has dedicated fields for AF statistics but these are never populated.

## Detailed Issues by Component:

### SoftISP Pipeline/Virtual Camera (`src/libcamera/pipeline/softisp/`):

1. **Missing IPA Integration**: The `processWithIPA()` method in `virtual_camera.cpp` is not properly called during frame processing.

2. **Incomplete Frame Processing**: The main frame processing loop doesn't integrate with the IPA module for actual image processing.

3. **No AF Support**: No auto-focus functionality is implemented in the virtual camera.

### SoftISP IPA Module (`src/ipa/softisp/`):

1. **Missing processFrame Implementation**: Only the method signature exists in the header, but no implementation.

2. **Incomplete ONNX Processing**: Only one stage of processing is implemented (`processStats`), but the dual-stage architecture (stats processing and frame processing) is not complete.

3. **No AF Integration**: The IPA module doesn't include any AF-related processing.

### AF Integration (`src/ipa/libipa/` and `src/libcamera/software_isp/`):

1. **Uninitialized Variables**: The AF statistics accumulation variables are not properly initialized.

2. **Missing Statistics Conversion**: The accumulated AF gradient values are never converted to normalized AF statistics.

3. **Incomplete Integration**: The calculated AF statistics are never passed to the simple pipeline AF algorithm.

## Recommendations:

1. **Fix AF Statistics Calculation**:
   - Initialize `afGradientSum_` and `afPixelCount_` to zero in the constructor
   - Add code in `finishFrame` to convert accumulated gradient values to normalized AF statistics
   - Set the AF statistics fields in the shared `SwIspStats` structure

2. **Complete SoftISP IPA Implementation**:
   - Implement the `processFrame()` method to handle Bayer to RGB/YUV conversion using the ONNX applier engine
   - Integrate both stages of processing (processStats and processFrame)

3. **Complete Virtual Camera IPA Integration**:
   - Properly call the IPA processing in the frame processing loop
   - Ensure the IPA interface is correctly set up and used

4. **Add AF Support**:
   - Integrate AF statistics generation in the virtual camera for testing
   - Ensure AF controls are properly handled and passed between components

## Summary:

The implementation shows a well-structured framework with clear separation of concerns, but critical functionality is missing or incomplete. The AF statistics calculation is particularly problematic, with uninitialized variables and missing conversion logic. The SoftISP IPA module is also incomplete, missing the core `processFrame()` implementation. These issues make the system non-functional despite having a good architectural foundation.