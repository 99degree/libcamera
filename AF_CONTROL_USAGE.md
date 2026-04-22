# AF ControlList Usage Guide

## Overview
The `AfAlgo` class now supports reading AF parameters from `ControlList`, allowing dynamic updates without reloading configuration files.

## Available Controls

### Standard Controls (libcamera)
| Control | Type | Description | Values |
|---------|------|-------------|--------|
| `controls::AfMode` | int32 | AF Mode | 0=Manual, 1=Auto, 2=Continuous |
| `controls::LensPosition` | float | Manual focus position | Dioptres (e.g., 1.0 = 1m) |

### Custom Controls (SoftISP)
Defined in `src/ipa/libipa/af_controls.h`:
| Control | Type | Description | Default |
|---------|------|-------------|---------|
| `controls::CustomAfFocusMin` | float | Minimum focus (dioptres) | 0.0 (infinity) |
| `controls::CustomAfFocusMax` | float | Maximum focus (dioptres) | 12.0 (8cm) |
| `controls::CustomAfStepCoarse` | float | Fast scan step size | 1.0 |
| `controls::CustomAfStepFine` | float | Precision step size | 0.25 |
| `controls::CustomAfPdafGain` | float | PDAF loop gain | -0.02 |
| `controls::CustomAfPdafSquelch` | float | Minimum PDAF movement | 0.125 |

## Usage Examples

### Example 1: Set Auto-Focus Mode
```cpp
#include <libcamera/control_list.h>
#include <libcamera/control_ids.h>
#include "libipa/af_algo.h"

libipa::AfAlgo afAlgo;

// Create control list
ControlList controls;
controls.set(controls::AfMode, 1); // Auto mode

// Apply to AF algorithm
afAlgo.handleControls(controls);

// Now process frames
float contrast = 0.5f; // From stats
bool updated = afAlgo.process(contrast);
if (updated) {
    int32_t vcm = afAlgo.getLensPosition();
    // Send vcm to hardware
}
```

### Example 2: Manual Focus
```cpp
ControlList controls;
controls.set(controls::AfMode, 0); // Manual mode
controls.set(controls::LensPosition, 2.5f); // Focus at 40cm (2.5 dioptres)

afAlgo.handleControls(controls);
// Lens will move to 2.5 dioptres immediately
```

### Example 3: Configure Focus Range
```cpp
ControlList controls;
controls.set(controls::CustomAfFocusMin, 0.0f);   // Infinity
controls.set(controls::CustomAfFocusMax, 8.0f);   // 12.5cm
controls.set(controls::CustomAfStepCoarse, 1.5f); // Faster scanning

afAlgo.handleControls(controls);
```

### Example 4: Continuous AF with Custom PDAF Tuning
```cpp
ControlList controls;
controls.set(controls::AfMode, 2); // Continuous mode
controls.set(controls::CustomAfPdafGain, -0.03f); // Stronger PDAF
controls.set(controls::CustomAfPdafSquelch, 0.1f); // Reduce wobble

afAlgo.handleControls(controls);
```

### Example 5: In SoftISP Pipeline
```cpp
void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId, 
                           const ControlList &sensorControls)
{
    // 1. Handle AF controls (Mode, Range, Speed, etc.)
    handleAfControls(sensorControls);
    
    // 2. Extract focus metrics (contrast, phase)
    float contrast = extractContrast(sensorControls);
    float phase = extractPhase(sensorControls);
    
    // 3. Run AF algorithm
    bool updated = afAlgo_.process(contrast, phase, 0.8f);
    
    // 4. Output lens position
    if (updated) {
        int32_t vcm = afAlgo_.getLensPosition();
        result.set(softisp::controls::focusPosition, vcm);
    }
}
```

## Control Flow Diagram
```
Application
    │
    ├─> ControlList (AfMode, LensPosition, CustomAf*)
    │
    ▼
SoftIsp::processStats()
    │
    ├─> handleAfControls()
    │     └─> afAlgo_.handleControls()
    │           ├─> Parse controls
    │           ├─> Update internal state
    │           └─> Trigger mode changes
    │
    ├─> extractContrast()/extractPhase()
    │
    ├─> afAlgo_.process(contrast, phase, conf)
    │     ├─> Hill-climbing (Auto mode)
    │     ├─> PDAF correction (Continuous mode)
    │     └─> Calculate new lens position
    │
    └─> Output: focusPosition (VCM value)
          │
          ▼
      Pipeline (V4L2 write)
```

## Future: Proper Control IDs
Currently using temporary IDs in `af_controls.h`. To make these official:

1. **Add to libcamera/controls.h**:
   ```cpp
   // In controls.h
   CustomAfFocusMin = 0x80010001,
   CustomAfFocusMax = 0x80010002,
   // ...
   ```

2. **Or define in softisp.mojom**:
   ```mojom
   namespace libcamera.ipa.softisp {
       control afFocusMin : float;
       control afFocusMax : float;
       // ...
   }
   ```

3. **Regenerate** and replace `CustomAf*` with generated IDs.

## Notes
- Controls are processed **every frame** in `processStats()`
- Only **changed values** trigger updates (efficiency)
- Invalid values are **clamped** to safe ranges
- Logging at `Debug` level for parameter changes
