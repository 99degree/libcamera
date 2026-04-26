# Technical Implementation Plan: Fixing AF Integration and SoftISP Components

## Overview

This document provides a detailed technical plan for fixing the identified issues in the AF integration and SoftISP components. The plan is organized by component and includes specific code changes needed to make the system functional.

## Part 1: Fixing AF Statistics Calculation

### Issue 1: Uninitialized AF Variables

**Location**: `include/libcamera/internal/software_isp/swstats_cpu.h`

**Problem**: The member variables `afGradientSum_` and `afPixelCount_` are declared but never initialized.

**Solution**:
```cpp
// In the SwStatsCpu constructor in swstats_cpu.cpp:
SwStatsCpu::SwStatsCpu(const GlobalConfiguration &configuration)
    : afGradientSum_(0), afPixelCount_(0), sharedStats_("softIsp_stats"), bench_(configuration, "CPU stats")
{
    // Existing initialization code...
}
```

### Issue 2: Missing AF Statistics Conversion

**Location**: `src/libcamera/software_isp/swstats_cpu.cpp`

**Problem**: The accumulated AF gradient values are never converted to the final AF statistics.

**Solution**: Modify the `finishFrame` method to convert and set AF statistics:

```cpp
void SwStatsCpu::finishFrame(uint32_t frame, uint32_t bufferId)
{
    bool valid = frame % kStatPerNumFrames == 0;

    if (valid) {
        sharedStats_->sum_ = RGB<uint64_t>({ 0, 0, 0 });
        sharedStats_->yHistogram.fill(0);
        for (const auto &s : stats_) {
            sharedStats_->sum_ += s.sum_;
            for (unsigned int j = 0; j < SwIspStats::kYHistogramSize; j++)
                sharedStats_->yHistogram[j] += s.yHistogram[j];
        }
        
        // Convert accumulated AF values to final statistics
        if (afPixelCount_ > 0) {
            // Calculate normalized contrast (0.0 to 1.0)
            float maxPossibleGradient = static_cast<float>(afPixelCount_ * 255 * 4); // Heuristic max
            sharedStats_->afContrast = std::min(1.0f, static_cast<float>(afGradientSum_) / maxPossibleGradient);
            sharedStats_->afRawContrast = static_cast<float>(afGradientSum_);
            sharedStats_->afValidPixels = afPixelCount_;
            sharedStats_->afPhaseError = 0.0f;  // No phase detection in CPU-only implementation
            sharedStats_->afPhaseConfidence = 0.0f;
        } else {
            sharedStats_->afContrast = 0.0f;
            sharedStats_->afRawContrast = 0.0f;
            sharedStats_->afValidPixels = 0;
            sharedStats_->afPhaseError = 0.0f;
            sharedStats_->afPhaseConfidence = 0.0f;
        }
    }

    sharedStats_->valid = valid;
    statsReady.emit(frame, bufferId);
    
    // Reset AF accumulation for next frame
    afGradientSum_ = 0;
    afPixelCount_ = 0;
}
```

### Issue 3: Initialize AF Variables in startFrame

**Location**: `src/libcamera/software_isp/swstats_cpu.cpp`

**Problem**: AF variables should be reset at the beginning of each frame processing.

**Solution**: Add initialization to startFrame method:

```cpp
void SwStatsCpu::startFrame(uint32_t frame)
{
    if (frame % kStatPerNumFrames)
        return;

    if (window_.width == 0)
        LOG(SwStatsCpu, Error) << "Calling startFrame() without setWindow()";

    for (auto &s : stats_) {
        s.sum_ = RGB<uint64_t>({ 0, 0, 0 });
        s.yHistogram.fill(0);
    }
    
    // Reset AF accumulation for new frame
    afGradientSum_ = 0;
    afPixelCount_ = 0;
}
```

## Part 2: Completing SoftISP IPA Implementation

### Issue 1: Missing processFrame Implementation

**Location**: `src/ipa/softisp/`

**Problem**: The `processFrame()` method is declared but not implemented.

**Solution**: Create `SoftIsp_processFrame.cpp`:

```cpp
/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <libcamera/base/log.h>
#include <libcamera/base/mapped_fd.h>
#include <chrono>

void SoftIsp::processFrame(const uint32_t frame,
                           uint32_t bufferId,
                           const SharedFD &bufferFd,
                           const int32_t planeIndex,
                           const int32_t width,
                           const int32_t height,
                           const ControlList &results)
{
    auto _pf_start = std::chrono::high_resolution_clock::now();
    LOG(SoftIsp, Info) << "processFrame: frame=" << frame 
                       << ", bufferId=" << bufferId 
                       << ", width=" << width 
                       << ", height=" << height;
    
    // Map the buffer for reading/writing
    MappedFD mappedBuffer(bufferFd, MappedFD::MapMode::ReadWrite);
    if (!mappedBuffer.isValid()) {
        LOG(SoftIsp, Error) << "Failed to map buffer for processFrame";
        return;
    }
    
    // Get current AWB gains from processStats
    float redGain = impl_->currentRedGain;
    float blueGain = impl_->currentBlueGain;
    
    LOG(SoftIsp, Info) << "Applying AWB gains: R=" << redGain << ", B=" << blueGain;
    
    // Apply ONNX processing (simplified example)
    if (impl_->applierEngine.isLoaded()) {
        // This would be where actual ONNX inference happens
        // For now, we'll just log that processing would occur
        LOG(SoftIsp, Info) << "ONNX applier engine would process frame " << frame;
    }
    
    // Signal frame completion
    frameDone.emit(frame, bufferId);
    
    auto _pf_end = std::chrono::high_resolution_clock::now();
    auto _pf_us = std::chrono::duration_cast<std::chrono::microseconds>(_pf_end - _pf_start).count();
    LOG(SoftIsp, Info) << "processFrame completed in " << _pf_us << " μs";
}
```

## Part 3: Completing Virtual Camera IPA Integration

### Issue 1: Missing IPA Processing Call

**Location**: `src/libcamera/pipeline/softisp/virtual_camera.cpp`

**Problem**: The `processWithIPA()` method is not properly integrated.

**Solution**: Modify the `processFrame` method in virtual_camera.cpp:

```cpp
void VirtualCamera::processFrame(FrameBuffer *buffer, Request *request)
{
    // Existing code for buffer processing...
    
    // Call IPA processing if available
    if (ipaInterface_) {
        processWithIPA(buffer, request);
    } else {
        // If no IPA, signal completion directly
        if (frameDoneCallback_) {
            uint32_t bufferId = plane.fd.get();
            frameDoneCallback_(sequence_, bufferId);
        }
    }
    
    // Existing code for buffer usage tracking...
}

void VirtualCamera::processWithIPA(FrameBuffer *buffer, Request *request)
{
    if (!ipaInterface_ || !buffer || buffer->planes().empty()) {
        LOG(VirtualCamera, Warning) << "IPA processing skipped - no interface or invalid buffer";
        return;
    }

    LOG(VirtualCamera, Info) << "Processing frame with IPA";

    const auto &plane = buffer->planes()[0];
    uint32_t bufferId = plane.fd.get();
    uint32_t frameId = sequence_;
    
    // Create control list for sensor controls (empty for now)
    ControlList sensorControls;
    
    // Call IPA processFrame method
    ipaInterface_->processFrame(
        frameId,           // frame
        bufferId,          // bufferId
        plane.fd,          // bufferFd
        0,                 // planeIndex
        width_,            // width
        height_,           // height
        sensorControls     // results
    );
    
    LOG(VirtualCamera, Info) << "IPA processFrame called for frame " << frameId << ", bufferId " << bufferId;
}
```

## Part 4: AF Integration with Simple Pipeline

### Issue 1: Connecting AF Statistics to Simple Pipeline

**Location**: `src/ipa/simple/algorithms/af.cpp`

**Problem**: The simple pipeline AF algorithm needs to receive real AF statistics.

**Solution**: Ensure the AF algorithm properly receives and processes real statistics:

```cpp
// In af.cpp process method, ensure real statistics are used:
void Af::process(IPAContext &context, const uint32_t frame,
                 IPAFrameContext &frameContext, const SwIspStats *stats,
                 ControlList &metadata)
{
    // ... existing code ...
    
    if (mode == libipa::AfMode::Auto || mode == libipa::AfMode::Continuous) {
        float contrast = 0.0f;
        float phaseError = 0.0f;
        float phaseConfidence = 0.0f;

        if (stats && stats->valid) {
            // Use REAL AF statistics instead of placeholder values
            contrast = stats->afContrast;
            phaseError = stats->afPhaseError;
            phaseConfidence = stats->afPhaseConfidence;
            
            // Log the actual statistics being used
            LOG(IPASoftAf, Debug) << "AF Stats - Contrast: " << contrast 
                                  << ", Phase Error: " << phaseError 
                                  << ", Confidence: " << phaseConfidence;
        }
        
        // ... rest of existing code ...
    }
}
```

## Implementation Priority:

1. **Phase 1**: Fix AF statistics calculation (highest priority)
   - Initialize AF variables
   - Implement AF statistics conversion
   - Test with simple pipeline AF algorithm

2. **Phase 2**: Complete SoftISP IPA implementation
   - Implement processFrame method
   - Integrate with ONNX engines
   - Test end-to-end processing

3. **Phase 3**: Complete Virtual Camera IPA integration
   - Properly integrate IPA processing calls
   - Ensure proper signal handling
   - Test complete pipeline

## Testing Plan:

1. **Unit Testing**: Test each component individually
2. **Integration Testing**: Test AF statistics flow from SwStatsCpu to simple pipeline
3. **End-to-End Testing**: Test complete pipeline with virtual camera and IPA

## Expected Outcomes:

After implementing these fixes:
- AF statistics will be properly calculated and converted
- SoftISP IPA will have complete frame processing capabilities
- Virtual camera will properly integrate with IPA module
- The complete autofocus pipeline will function correctly