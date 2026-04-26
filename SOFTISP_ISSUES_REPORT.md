# SoftISP Implementation Production Readiness Issues

## Current Issues Identified

### 1. AF Statistics Integration Issues

**Problem**: The SwStatsCpu class accumulates AF gradient data but doesn't properly populate the AF fields in SwIspStats.

**Specific Issues**:
- Constructor doesn't initialize AF statistics variables (`afGradientSum_`, `afPixelCount_`)
- `startFrame()` function doesn't reset AF statistics variables
- `finishFrame()` function doesn't populate AF fields in shared statistics
- No proper integration between SwStatsCpu and AfStatsCalculator

**Files Affected**:
- `src/libcamera/software_isp/swstats_cpu.cpp`
- `src/libcamera/software_isp/swstats_cpu.h`

### 2. AF Algorithm Integration Issues

**Problem**: The AF algorithm is implemented but not fully integrated with actual frame data.

**Specific Issues**:
- AfStatsCalculator not connected to actual frame data
- No PDAF integration with phase detection data
- Limited real-world testing with VCM hardware

**Files Affected**:
- `src/ipa/libipa/af_stats.cpp`
- `src/ipa/libipa/af_stats.h`
- `src/ipa/simple/algorithms/af.cpp`
- `src/ipa/simple/algorithms/af.h`

## Required Fixes for Production Readiness

### Phase 1: Critical Fixes (Immediate)

1. **Fix SwStatsCpu AF Statistics Integration**
   - Initialize AF statistics variables in constructor
   - Reset AF statistics in `startFrame()`
   - Populate AF fields in `finishFrame()`
   - Ensure proper synchronization of AF data

2. **Connect AfStatsCalculator to Frame Data**
   - Modify SwStatsCpu to properly populate SwIspStats AF fields
   - Ensure data consistency between SwStatsCpu and AfStatsCalculator

### Phase 2: Architecture Refinement (Short-term)

3. **Consolidate Implementation Files**
   - Merge split implementation files into coherent units
   - Ensure proper initialization and cleanup
   - Add comprehensive error handling

4. **Enhance Error Handling and Recovery**
   - Add proper error handling for AF algorithm
   - Implement robust error recovery mechanisms
   - Add detailed logging and debugging capabilities

### Phase 3: Production Hardening (Medium-term)

5. **Complete PDAF Integration**
   - Add phase detection support to SwStatsCpu
   - Implement PDAF statistics collection from real sensors
   - Add VCM driver interface for lens position control

6. **Performance Optimization**
   - Optimize AF algorithm for real-time processing
   - Implement cross-platform compatibility
   - Add comprehensive testing and validation

## Implementation Plan

### 1. Fix SwStatsCpu Constructor and AF Variables

```cpp
// In SwStatsCpu constructor
SwStatsCpu::SwStatsCpu(const GlobalConfiguration &configuration)
    : sharedStats_("softIsp_stats"), bench_(configuration, "CPU stats"),
      afGradientSum_(0), afPixelCount_(0)  // Initialize AF variables
{
    if (!sharedStats_)
        LOG(SwStatsCpu, Error)
            << "Failed to create shared memory for statistics";
}

// In startFrame function
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
    
    // Reset AF statistics for new frame
    afGradientSum_ = 0;
    afPixelCount_ = 0;
}
```

### 2. Fix SwStatsCpu finishFrame Function

```cpp
// In finishFrame function
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
        
        // Calculate and populate AF statistics
        if (afPixelCount_ > 0) {
            // Calculate raw contrast as average gradient magnitude
            float rawContrast = static_cast<float>(afGradientSum_) / afPixelCount_;
            
            // Normalize contrast to 0.0-1.0 range (heuristic)
            float maxExpected = 255.0f * 4.0f;
            float normalizedContrast = std::min(rawContrast / maxExpected, 1.0f);
            
            // Populate AF statistics fields
            sharedStats_->afRawContrast = rawContrast;
            sharedStats_->afContrast = normalizedContrast;
            sharedStats_->afValidPixels = afPixelCount_;
            sharedStats_->afPhaseError = 0.0f;  // No phase detection data
            sharedStats_->afPhaseConfidence = 0.0f;
        } else {
            // No valid AF data
            sharedStats_->afRawContrast = 0.0f;
            sharedStats_->afContrast = 0.0f;
            sharedStats_->afValidPixels = 0;
            sharedStats_->afPhaseError = 0.0f;
            sharedStats_->afPhaseConfidence = 0.0f;
        }
        
        // Reset AF accumulation for next frame
        afGradientSum_ = 0;
        afPixelCount_ = 0;
    } else {
        // For non-AF frames, ensure AF fields are cleared
        sharedStats_->afRawContrast = 0.0f;
        sharedStats_->afContrast = 0.0f;
        sharedStats_->afValidPixels = 0;
        sharedStats_->afPhaseError = 0.0f;
        sharedStats_->afPhaseConfidence = 0.0f;
    }

    sharedStats_->valid = valid;
    statsReady.emit(frame, bufferId);
}
```

## Testing and Validation Requirements

### Unit Testing
1. Test AF algorithm with various focus scenarios
2. Validate VirtualCamera frame generation
3. Verify IPA module processing accuracy
4. Check error handling and recovery

### Integration Testing
1. End-to-end pipeline testing
2. Real camera integration validation
3. Performance benchmarking
4. Cross-platform compatibility testing

### Performance Targets
- Frame processing: < 150μs overhead
- Memory usage: < 50MB peak usage
- Real-time processing: 1920x1080 @ 30 FPS
- Error recovery: < 1 second for transient failures

This plan ensures a production-ready SoftISP implementation that follows libcamera patterns and conventions while providing robust, maintainable code.