# Auto-Focus Integration for libcamera/libipa

## Overview

This document describes the AF (Auto-Focus) algorithm integration for the simple IPA pipeline. The implementation provides a hardware-agnostic AF solution that can work with any camera hardware that supports a VCM (Voice Coil Motor) lens.

## Components

### 1. AfAlgo (Hardware-Agnostic AF Algorithm)
**Location**: `src/ipa/libipa/af_algo.cpp|h`

The core AF control loop that implements:
- **Three modes**: Manual, Auto (one-shot scan), Continuous
- **Two detection methods**: 
  - CDAF (Contrast Detection AF) - hill climbing algorithm
  - PDAF (Phase Detection AF) - when phase data is available
- **Configurable parameters**: Focus range, step sizes, loop gains

### 2. AfStatsCalculator (CPU-based Statistics)
**Location**: `src/ipa/libipa/af_stats.cpp|h`

Calculates contrast metrics from raw image data:
- **Methods**: Sobel, Laplacian, Variance
- **ROI support**: Single or multiple weighted regions
- **Output**: Normalized contrast score (0.0-1.0), raw contrast value

### 3. Af IPA Algorithm (Simple Pipeline Integration)
**Location**: `src/ipa/simple/algorithms/af.cpp|h`

Wraps AfAlgo for the simple IPA pipeline:
- Integrates with ControlList for AF controls
- Outputs `controls::LensPosition` and `controls::AfState`
- Currently uses placeholder contrast (needs frame data access)

## Usage

### Configuration

Create an AF configuration file (e.g., `af_config.ini`):

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

### Control Flow

1. **Set AF Mode** (via ControlList):
   ```cpp
   controls.add(controls::AfMode, static_cast<int32_t>(AfMode::Auto));
   ```

2. **Algorithm Processes Frames**:
   - Calculates contrast from frame data
   - Runs hill climbing or PDAF correction
   - Updates lens position

3. **Read Lens Position**:
   ```cpp
   float lensPos = request.metadata.get(controls::LensPosition);
   ```

### AF Modes

#### Manual Mode
Direct lens control via `controls::LensPosition`:
```cpp
controls.add(controls::AfMode, 0); // Manual
controls.add(controls::LensPosition, 0.5f); // 50% of range
```

#### Auto Mode (One-shot)
Scans to find peak contrast, then stops:
```cpp
controls.add(controls::AfMode, 1); // Auto
// Algorithm scans and locks on peak
```

#### Continuous Mode
Continuously tracks focus (hybrid PDAF+CDAF):
```cpp
controls.add(controls::AfMode, 2); // Continuous
// Algorithm continuously adjusts lens position
```

## Integration Status

### Current Implementation
- ✅ AfAlgo class (core algorithm)
- ✅ AfStatsCalculator (CPU statistics)
- ✅ Af IPA algorithm wrapper
- ✅ Control handling (AfMode, LensPosition)
- ✅ Configuration file support

### TODO / Next Steps

1. **Frame Data Access**: The current implementation uses placeholder contrast values. To make it fully functional:
   - Modify `process()` to receive raw frame data
   - Or calculate AF stats in a separate pass before `process()`
   - Connect `AfStatsCalculator` to actual frame data

2. **PDAF Integration**: Add support for phase detection statistics:
   - Define PDAF statistics structure
   - Calculate phase error from PDAF pixels
   - Pass phase data to `AfAlgo::process()`

3. **VCM Driver Interface**: Connect lens position to hardware:
   - Map `controls::LensPosition` to VCM driver commands
   - Handle VCM calibration data
   - Add VCM-specific controls (focus min/max)

4. **Testing**: Add comprehensive tests:
   - Unit tests for AfAlgo
   - Integration tests with simulated frames
   - Real hardware testing with VCM lens

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Camera Application                    │
│  (sets AfMode, reads LensPosition, AfState)             │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│                    ControlList                           │
│  (AfMode, LensPosition, AfState, etc.)                  │
└─────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────┐
│              Af IPA Algorithm (af.cpp)                   │
│  - Handles ControlList                                   │
│  - Coordinates AfAlgo and AfStatsCalculator              │
└─────────────────────────────────────────────────────────┘
         │                                    │
         ▼                                    ▼
┌──────────────────────┐         ┌──────────────────────────────┐
│    AfAlgo            │         │   AfStatsCalculator          │
│  (libipa/af_algo)    │◄────────│  (libipa/af_stats)           │
│  - Control loop      │         │  - Contrast calculation      │
│  - Mode management   │         │  - ROI support               │
│  - PDAF+CDAF hybrid  │         │  - Multiple methods          │
└──────────────────────┘         └──────────────────────────────┘
         │                                    │
         ▼                                    ▼
┌──────────────────────┐         ┌──────────────────────────────┐
│   VCM Driver         │         │   Raw Frame Data             │
│  (controls::Lens     │         │   (from camera sensor)       │
│   Position)          │         │                              │
└──────────────────────┘         └──────────────────────────────┘
```

## Files Modified/Created

### New Files
- `src/ipa/libipa/af_stats.h` - AF statistics calculator header
- `src/ipa/libipa/af_stats.cpp` - AF statistics calculator implementation
- `src/ipa/simple/algorithms/af.h` - AF IPA algorithm header
- `src/ipa/simple/algorithms/af.cpp` - AF IPA algorithm implementation

### Modified Files
- `src/ipa/libipa/meson.build` - Added af_stats.cpp
- `src/ipa/simple/algorithms/meson.build` - Added af.cpp

## Building

The AF integration is built as part of the normal libcamera build:

```bash
meson setup build
ninja -C build
```

## Testing

### Unit Test Example
```cpp
#include "libipa/af_algo.h"
#include "libipa/af_stats.h"

TEST(AfAlgo, BasicScan) {
  libipa::AfAlgo af;
  af.setRange(0.0f, 12.0f, 1.0f);
  af.setMode(libipa::AfMode::Auto);
  
  // Simulate contrast values during scan
  float contrast = 0.1f;
  for (int i = 0; i < 100; i++) {
    contrast += 0.01f; // Simulate increasing contrast
    af.process(contrast, 0.0f, 0.0f);
  }
  
  EXPECT_TRUE(af.hasNewPosition());
}
```

## References

- [AfAlgo API](af_algo.h) - Core AF algorithm documentation
- [AfStatsCalculator API](af_stats.h) - Statistics calculation documentation
- [Android AF Controls](https://developer.android.com/reference/android/hardware/camera2/CaptureRequest#ANDROID_CONTROL_AF_MODE) - Standard AF control definitions
