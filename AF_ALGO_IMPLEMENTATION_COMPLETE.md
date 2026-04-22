# AF Algorithm Implementation Complete ✅

## Summary
Successfully extracted and adapted the Raspberry Pi AF algorithm into a **hardware-agnostic library** (`libipa::AfAlgo`) under `src/ipa/libipa/`.

## What Was Created

### Files
- **`src/ipa/libipa/af_algo.h`** - Public API header (200 lines)
  - `AfMode` enum (Manual, Auto, Continuous)
  - `AfState` enum (Idle, Scanning, Focusing, Failed)
  - `AfAlgo` class with full configuration and processing API
  
- **`src/ipa/libipa/af_algo.cpp`** - Implementation (300 lines)
  - Hill-climbing CDAF logic
  - PDAF phase detection loop
  - State machine for auto-focus scans
  - Config file loading/saving

### Features Implemented
✅ **Three AF Modes:**
- **Manual**: Direct lens control via `setLensPosition()`
- **Auto**: One-shot scan to find peak contrast
- **Continuous**: Real-time tracking with PDAF + CDAF hybrid

✅ **PDAF Support:**
- Phase error input (pixels)
- Confidence weighting
- Loop gain and squelch parameters
- Fallback to CDAF when confidence is low

✅ **CDAF Support:**
- Contrast-based hill climbing
- Peak detection with ratio threshold
- Adaptive step size (coarse → fine)
- Retransmission logic for failed scans

✅ **Configuration:**
- API methods: `setRange()`, `setSpeed()`, `setPdafParams()`, etc.
- INI-style config file support
- Default values optimized for typical VCM lenses

✅ **Output:**
- Lens position in **dioptres** (scientific unit: 1/meters)
- Lens position in **VCM steps** (0-1023, hardware units)
- Smooth position transitions (prevents jitter)

## How It Works

### Input (The "Eyes")
The algorithm receives **focus metrics** from any source:
```cpp
bool process(float contrast, float phase = 0.0f, float conf = 0.0f);
```
- **`contrast`**: 0.0 to 1.0 (higher = sharper image)
- **`phase`**: Phase detection error in pixels (positive = lens too close)
- **`conf`**: Confidence in phase data (0.0 to 1.0)

### Processing (The "Brain")
1. **Auto Mode**: Scans lens from near to far, tracking contrast peak
2. **Continuous Mode**: Uses PDAF for fast correction + CDAF for fine-tuning
3. **Manual Mode**: No automatic updates (direct control only)

### Output (The "Muscle")
```cpp
int32_t getLensPosition() const; // VCM value (0-1023)
float getTargetDioptres() const; // Scientific focus value
AfState getState() const;        // Current state
```

## Next Steps: Integration with SoftISP

### 1. Create Stat Extractor
Implement `src/ipa/softisp/af_extractor.cpp`:
- Extract contrast from ONNX output or hardware stats
- Extract phase if PDAF data available
- Return `float contrast, float phase, float conf`

### 2. Integrate into SoftIsp
Update `src/ipa/softisp/softisp.cpp`:
```cpp
#include "libipa/af_algo.h"

class SoftIsp {
    libipa::AfAlgo afAlgo_;
    
    void processStats(...) {
        // Extract stats
        float contrast = extractor.extractContrast(stats);
        float phase = extractor.extractPhase(stats);
        
        // Run AF algorithm
        bool updated = afAlgo_.process(contrast, phase, 0.8f);
        
        // Output to pipeline
        if (updated) {
            int32_t vcm = afAlgo_.getLensPosition();
            result.set(softisp::controls::focusPosition, vcm);
        }
    }
};
```

### 3. Update Build System
In `src/ipa/softisp/meson.build`:
```meson
softisp_deps += [libipa_dep]
```

### 4. Define Mojom Control
In `include/libcamera/ipa/softisp.mojom`:
```mojom
namespace libcamera.ipa.softisp {
    control focusPosition : int32;
}
```

## Configuration Example

Create `softisp_af_config.txt`:
```ini
[af]
focus_min = 0.0        # Infinity
focus_max = 12.0       # 8cm closest focus
focus_default = 1.0    # 1 meter

step_coarse = 1.0      # Fast scan speed
step_fine = 0.25       # Precision speed
max_slew = 2.0         # Max movement per frame

pdaf_gain = -0.02      # PDAF correction strength
pdaf_squelch = 0.125   # Minimum movement threshold
pdaf_conf_thresh = 0.1 # Trust PDAF if conf > 0.1

contrast_ratio = 0.75  # Peak detection threshold
conf_epsilon = 8.0     # Confidence smoothing
skip_frames = 5        # Update every 5 frames
```

Load in code:
```cpp
afAlgo_.loadConfig("softisp_af_config.txt");
```

## Testing

### Unit Test (Mock Data)
```cpp
TEST(AfAlgo, HillClimbing) {
    AfAlgo af;
    af.setRange(0.0f, 12.0f, 1.0f);
    af.setMode(AfMode::Auto);
    
    // Simulate contrast parabola: peak at 6.0 dioptres
    for (int i = 0; i < 100; i++) {
        float pos = af.getTargetDioptres();
        float contrast = 1.0f - abs(pos - 6.0f) / 6.0f; // Parabola
        
        af.process(contrast);
        
        if (af.getState() == AfState::Idle) break; // Peak found
    }
    
    EXPECT_NEAR(af.getTargetDioptres(), 6.0f, 0.5f);
}
```

### Integration Test (SoftISP)
- Run `softisp-test-app` with `--pipeline dummysoftisp`
- Feed synthetic contrast values
- Verify `focusPosition` control is generated

### Hardware Test
- Connect VCM lens to camera
- Run `camera-test` with `AfMode=Auto`
- Observe lens movement and focus convergence

## Comparison with Original RPi Code

| Feature | Original RPi | New libipa::AfAlgo |
|---------|--------------|-------------------|
| **Dependencies** | RPi Controller framework | None (standalone) |
| **Config** | YAML parsing | API + simple INI |
| **Stats Format** | Custom `StatisticsPtr` | Simple floats |
| **Portability** | RPi only | Any hardware |
| **Lines of Code** | ~900 | ~300 (stripped down) |
| **Modes** | Manual, Auto, Continuous | Manual, Auto, Continuous ✅ |
| **PDAF** | Yes | Yes ✅ |
| **CDAF** | Yes | Yes ✅ |

## Conclusion

The AF algorithm is now a **reusable, public component** that can be:
- Used by SoftISP for auto-focus
- Shared with other IPA modules
- Easily tested and tuned
- Adapted for different lenses/sensors

**Status:** ✅ Implementation Complete, Ready for Integration
