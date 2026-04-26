# AF Algorithm Integration - Complete Implementation

## Overview

This document describes the complete AF (Auto-Focus) integration for libcamera's simple IPA pipeline. The implementation provides a hardware-agnostic AF solution that works with any camera hardware supporting a VCM lens.

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                    Camera Application                            │
│  (sets AfMode, reads LensPosition, AfState)                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    ControlList                                   │
│  (controls::AfMode, controls::LensPosition, controls::AfState)  │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              Af IPA Algorithm (simple/algorithms/af.cpp)         │
│  - Handles ControlList                                           │
│  - Coordinates AfAlgo and AfStatsCalculator                      │
│  - Outputs lens position via metadata                            │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ reads SwIspStats
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  SwIspStats Structure                            │
│  - afContrast: Normalized contrast (0.0-1.0)                    │
│  - afRawContrast: Raw contrast value                            │
│  - afValidPixels: Number of pixels used                         │
│  - afPhaseError: Phase detection error (if available)           │
│  - afPhaseConfidence: Confidence in phase data                  │
└─────────────────────────────────────────────────────────────────┘
                              ▲
                              │ calculated by
                              │
┌─────────────────────────────────────────────────────────────────┐
│              Stats Provider (SwStatsCpu or SoftISP)              │
│  - Calculates contrast from raw frame data                      │
│  - Optionally calculates phase error for PDAF                   │
│  - Populates SwIspStats.af* fields                              │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│              AfAlgo (libipa/af_algo.cpp)                         │
│  - Control loop: Manual, Auto, Continuous modes                 │
│  - CDAF: Hill climbing algorithm                                │
│  - PDAF: Phase detection correction                             │
│  - Outputs lens position (VCM steps)                            │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ maps to
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              VCM Driver (controls::LensPosition)                 │
│  - Voice Coil Motor control                                     │
│  - Maps normalized position (0.0-1.0) to hardware steps         │
└─────────────────────────────────────────────────────────────────┘
```

## Components

### 1. Core Algorithm: `AfAlgo`
**Location**: `src/ipa/libipa/af_algo.cpp|h`

Hardware-agnostic AF control loop implementing:
- **Three modes**: Manual, Auto (one-shot scan), Continuous tracking
- **Two detection methods**:
  - CDAF (Contrast Detection): Hill climbing algorithm
  - PDAF (Phase Detection): When phase data is available
- **Configurable parameters**: Focus range, step sizes, loop gains

### 2. Statistics Calculator: `AfStatsCalculator`
**Location**: `src/ipa/libipa/af_stats.cpp|h`

CPU-based contrast calculation:
- **Methods**: Sobel, Laplacian, Variance
- **ROI support**: Single or multiple weighted regions
- **Output**: Normalized contrast score (0.0-1.0)

### 3. IPA Algorithm: `Af`
**Location**: `src/ipa/simple/algorithms/af.cpp|h`

Simple pipeline integration:
- Wraps `AfAlgo` for IPA framework
- Handles AF controls via ControlList
- Outputs `controls::LensPosition` and `controls::AfState`
- Reads AF stats from `SwIspStats`

### 4. Stats Structure: `SwIspStats`
**Location**: `include/libcamera/internal/software_isp/swisp_stats.h`

Extended to include AF fields:
```cpp
struct SwIspStats {
    // ... existing fields (sum_, yHistogram) ...
    
    float afContrast;         // Normalized contrast (0.0-1.0)
    float afRawContrast;      // Raw contrast value
    uint32_t afValidPixels;   // Number of pixels used
    float afPhaseError;       // Phase detection error
    float afPhaseConfidence;  // Phase confidence (0.0-1.0)
};
```

## Integration Steps

### Step 1: Calculate AF Stats in Your Stats Provider

You need to calculate AF statistics from the raw frame data. This can be done in:

#### Option A: In SwStatsCpu (CPU-based stats)
Modify `src/libcamera/software_isp/swstats_cpu.cpp` to calculate AF contrast while processing lines.

#### Option B: In SoftISP (your implementation)
Add AF stats calculation in your SoftISP frame processing:

```cpp
#include "libcamera/internal/software_isp/swisp_stats.h"
#include "libipa/af_stats.h"

void SoftIspImpl::processFrame(uint32_t frame, const uint8_t *rawFrame,
                               uint32_t width, uint32_t height, uint32_t stride)
{
    SwIspStats stats = {};
    stats.valid = true;
    
    // Calculate color and histogram stats (existing code)
    calculateColorStats(rawFrame, width, height, stride, &stats);
    
    // Calculate AF stats (NEW)
    libipa::AfStatsCalculator afCalc;
    afCalc.setMethod(libipa::AfStatsCalculator::Method::Sobel);
    
    // Set up ROIs
    Rectangle mainRoi(width/4, height/4, width/2, height/2);
    afCalc.addWeightedRoi(mainRoi, 1.0f);
    
    libipa::AfStats afStats = afCalc.calculate(rawFrame, width, height, stride, 8);
    
    stats.afContrast = afStats.contrast;
    stats.afRawContrast = afStats.rawContrast;
    stats.afValidPixels = afStats.validPixels;
    
    // Write stats to shared memory
    writeStatsToSharedMemory(stats);
}
```

#### Option C: Use the Example Code
See `docs/af_stats_example.cpp` for a complete example.

### Step 2: Enable AF Algorithm in IPA

Add the AF algorithm to your tuning file or enable it in the simple pipeline:

```yaml
algorithms:
  - Af:
      enabled: true
  - Agc:
  - Awb:
```

### Step 3: Control AF from Application

```cpp
// Set AF mode to Auto (one-shot scan)
ControlList controls;
controls.set(controls::AfMode, static_cast<int32_t>(libcamera::controls::AfMode::Auto));

// Or set to Continuous mode
controls.set(controls::AfMode, static_cast<int32_t>(libcamera::controls::AfMode::Continuous));

// Or Manual mode with specific position
controls.set(controls::AfMode, static_cast<int32_t>(libcamera::controls::AfMode::Manual));
controls.set(controls::LensPosition, 0.5f); // 50% of range

// Submit request
camera->capture(request, &controls);

// Read AF state and lens position from results
int32_t afState = request.metadata.get(controls::AfState);
float lensPosition = request.metadata.get(controls::LensPosition);
```

### Step 4: Configure AF Parameters (Optional)

Create a configuration file (e.g., `/data/data/com.termux/files/home/libcamera/config/af_config.ini`):

```ini
[af]
# Focus range in dioptres (1/meters)
focus_min = 0.0
focus_max = 12.0
focus_default = 1.0

# Movement parameters
step_coarse = 1.0
step_fine = 0.25
max_slew = 2.0

# PDAF parameters (if using phase detection)
pdaf_gain = -0.02
pdaf_squelch = 0.125
pdaf_conf_thresh = 0.1

# CDAF parameters
contrast_ratio = 0.75
conf_epsilon = 8.0
skip_frames = 5
```

The algorithm automatically loads this file on startup.

## AF Modes Explained

### Manual Mode (AfMode::Manual)
Direct lens control via `controls::LensPosition`:
- Application sets lens position directly
- No automatic focusing
- Use case: Fixed focus, manual focus applications

### Auto Mode (AfMode::Auto)
One-shot scan to find peak contrast:
- Algorithm scans from near to far (or vice versa)
- Finds maximum contrast point
- Stops and locks on peak
- Use case: Still photography, single-shot focus

### Continuous Mode (AfMode::Continuous)
Continuous focus tracking:
- Algorithm continuously adjusts lens position
- Uses PDAF if available, falls back to CDAF
- Tracks moving subjects
- Use case: Video recording, continuous AF

## Files Modified/Created

### New Files
- `src/ipa/libipa/af_stats.h` - AF statistics calculator header
- `src/ipa/libipa/af_stats.cpp` - AF statistics calculator implementation
- `src/ipa/simple/algorithms/af.h` - AF IPA algorithm header
- `src/ipa/simple/algorithms/af.cpp` - AF IPA algorithm implementation
- `docs/AF_INTEGRATION_COMPLETE.md` - This documentation
- `docs/af_stats_example.cpp` - Example code for AF stats calculation

### Modified Files
- `src/ipa/libipa/meson.build` - Added `af_stats.cpp`
- `src/ipa/simple/algorithms/meson.build` - Added `af.cpp`
- `include/libcamera/internal/software_isp/swisp_stats.h` - Added AF fields

## Next Steps for Full Functionality

### 1. Implement AF Stats Calculation
Choose one of the options above to calculate AF contrast from raw frames.

### 2. Add PDAF Support (Optional)
If your hardware supports phase detection:
- Extract PDAF pixels from the frame
- Calculate phase difference between left/right views
- Set `stats->afPhaseError` and `stats->afPhaseConfidence`

### 3. VCM Driver Integration
Map lens position to your VCM driver:
```cpp
// In your camera pipeline or VCM driver
float normalizedPos = request.metadata.get(controls::LensPosition);
int32_t vcmSteps = static_cast<int32_t>(normalizedPos * 1023);
vcmDriver->setFocus(vcmSteps);
```

### 4. Testing
- Test with different scenes and lighting conditions
- Tune AF parameters for your lens/sensor combination
- Verify focus accuracy with test charts

## Troubleshooting

### AF doesn't start
- Check that `Af` algorithm is enabled in the tuning file
- Verify that AF stats are being calculated (check logs for "AF stats:")
- Ensure `controls::AfMode` is set correctly

### AF oscillates or wobbles
- Increase `pdaf_squelch` or `skip_frames` in config
- Adjust `contrast_ratio` parameter
- Check for vibration or unstable lighting

### AF fails to find focus
- Verify focus range covers your use case
- Check that contrast is being calculated correctly
- Ensure sufficient scene contrast (AF needs edges!)

### Lens position doesn't move
- Check VCM driver integration
- Verify `controls::LensPosition` is being set
- Check that lens position control is passed to hardware

## References

- [AfAlgo API](../src/ipa/libipa/af_algo.h) - Core AF algorithm documentation
- [AfStatsCalculator API](../src/ipa/libipa/af_stats.h) - Statistics calculation
- [Android AF Controls](https://developer.android.com/reference/android/hardware/camera2/CaptureRequest#ANDROID_CONTROL_AF_MODE) - Standard AF controls
- [Example Code](af_stats_example.cpp) - Complete integration example
