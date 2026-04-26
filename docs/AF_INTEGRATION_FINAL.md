# AF Algorithm Integration - Final Implementation

## Summary

The AF (Auto-Focus) algorithm has been successfully integrated into the libcamera simple pipeline. The implementation provides automatic contrast-based autofocus that works with any camera hardware supporting a VCM lens.

## What Was Implemented

### 1. Core AF Algorithm (`src/ipa/libipa/af_algo.*`)
- **Already existed** - Hardware-agnostic AF control loop
- Supports Manual, Auto (one-shot), and Continuous modes
- Implements CDAF (Contrast Detection) and PDAF (Phase Detection)

### 2. AF IPA Algorithm (`src/ipa/simple/algorithms/af.*`)
- **Newly created** - Integrates AfAlgo into simple IPA pipeline
- Handles ControlList for AF controls
- Outputs `controls::LensPosition` and `controls::AfState`
- Reads AF contrast from `SwIspStats`

### 3. AF Statistics in SwStatsCpu (`src/libcamera/software_isp/swstats_cpu.*`)
- **Modified** - Added automatic contrast calculation
- Accumulates gradient magnitudes during line processing
- Calculates normalized contrast in `finishFrame()`
- Populates `SwIspStats` with AF fields:
  - `afContrast`: Normalized contrast (0.0-1.0)
  - `afRawContrast`: Raw gradient average
  - `afValidPixels`: Number of pixels used

### 4. Extended Statistics Structure (`include/libcamera/internal/software_isp/swisp_stats.h`)
- **Modified** - Added AF fields to `SwIspStats`

### 5. Build System
- **Modified** - Updated meson.build files to include new sources

## How It Works

```
Frame Processing Flow:
1. SwStatsCpu processes Bayer frame line-by-line
   - For each 2x2 pixel block:
     * Calculates color sums and histogram (existing)
     * Calculates gradient magnitude (NEW): |G-B| + |G2-R|
     * Accumulates: afGradientSum_ += gradient, afPixelCount_ += 2

2. finishFrame() calculates final AF stats:
   - avgGradient = afGradientSum_ / afPixelCount_
   - afContrast = clamp(avgGradient / 1024, 0.0, 1.0)
   - Stores in sharedStats_

3. Af IPA algorithm reads SwIspStats:
   - Gets afContrast from stats
   - Passes to AfAlgo::process(contrast, 0, 0)
   - AfAlgo runs hill climbing algorithm

4. AfAlgo outputs lens position:
   - Calculates optimal focus position
   - Sets controls::LensPosition in metadata
   - Application reads and sends to VCM driver
```

## Files Modified/Created

### Created:
- `src/ipa/simple/algorithms/af.h` - AF IPA algorithm header
- `src/ipa/simple/algorithms/af.cpp` - AF IPA algorithm implementation
- `src/ipa/libipa/af_stats.h` - AF stats calculator header (optional utility)
- `src/ipa/libipa/af_stats.cpp` - AF stats calculator implementation (optional)
- `docs/AF_INTEGRATION_FINAL.md` - This document

### Modified:
- `include/libcamera/internal/software_isp/swisp_stats.h` - Added AF fields
- `src/libcamera/software_isp/swstats_cpu.h` - Added AF accumulation variables
- `src/libcamera/software_isp/swstats_cpu.cpp` - Added gradient calculation in 5 functions
- `src/ipa/libipa/meson.build` - Added af_stats.cpp
- `src/ipa/simple/algorithms/meson.build` - Added af.cpp

## Usage

### Enable AF in Application

```cpp
#include <libcamera/controls.h>

// Set AF mode to Auto (one-shot scan)
ControlList controls;
controls.set(controls::AfMode, 
             static_cast<int32_t>(libcamera::controls::AfMode::Auto));

// Submit capture request
camera->capture(request, &controls);

// Read results
int32_t afState = request.metadata.get(controls::AfState);
float lensPosition = request.metadata.get(controls::LensPosition);
```

### AF Modes

- **Manual (0)**: Direct lens control via `controls::LensPosition`
- **Auto (1)**: One-shot scan to find peak contrast
- **Continuous (2)**: Continuous focus tracking

### Configuration (Optional)

Create `/data/data/com.termux/files/home/libcamera/config/af_config.ini`:

```ini
[af]
focus_min = 0.0      # Minimum focus distance in dioptres
focus_max = 12.0     # Maximum focus distance in dioptres
focus_default = 1.0  # Default starting position
step_coarse = 1.0    # Large step size for fast scanning
step_fine = 0.25     # Small step size for precision
max_slew = 2.0       # Maximum movement speed
```

## Testing

### Verify AF Stats Are Being Calculated

Check logs for AF statistics:
```bash
logcat | grep SwStatsCpu
# Should see gradient accumulation messages
```

### Test AF Mode Switching

```cpp
// Test Manual mode
controls.set(controls::AfMode, 0);
controls.set(controls::LensPosition, 0.5f);

// Test Auto mode
controls.set(controls::AfMode, 1);

// Test Continuous mode
controls.set(controls::AfMode, 2);
```

### Verify Lens Movement

Monitor `controls::LensPosition` in metadata:
```cpp
float pos = request.metadata.get(controls::LensPosition);
LOG(INFO) << "Lens position: " << pos;
```

## Limitations

1. **No PDAF Support**: Current implementation only uses CDAF (contrast detection). PDAF would require:
   - PDAF pixel extraction from sensor
   - Phase difference calculation
   - Integration into SwStatsCpu or separate PDAF stats provider

2. **Simple Gradient**: Uses simple horizontal+vertical difference instead of full Sobel filter for performance. Can be enhanced later.

3. **Fixed Normalization**: Contrast normalization uses fixed max value (1024). May need tuning for different sensors.

## Next Steps for Enhancement

1. **Add PDAF Support**:
   - Extract PDAF pixels from sensor
   - Calculate phase error
   - Set `stats->afPhaseError` and `stats->afPhaseConfidence`

2. **Better Gradient Calculation**:
   - Replace simple difference with Sobel filter
   - Add configurable AF ROI

3. **Performance Optimization**:
   - Use SIMD for gradient calculation
   - Downsample for faster processing

4. **VCM Driver Integration**:
   - Map `controls::LensPosition` to actual VCM steps
   - Add VCM calibration support

## Troubleshooting

### AF doesn't work
- Check that `Af` algorithm is enabled in tuning file
- Verify `controls::AfMode` is set correctly
- Check logs for "AF stats" messages

### Lens doesn't move
- Verify VCM driver is properly integrated
- Check that `controls::LensPosition` is being set
- Ensure lens position control reaches hardware

### Poor focus quality
- Adjust `focus_min` and `focus_max` in config
- Tune `step_coarse` and `step_fine` parameters
- Verify sufficient scene contrast (AF needs edges!)

## References

- [AfAlgo API](../src/ipa/libipa/af_algo.h)
- [Android AF Controls](https://developer.android.com/reference/android/hardware/camera2/CaptureRequest#ANDROID_CONTROL_AF_MODE)
- [Hill Climbing Algorithm](https://en.wikipedia.org/wiki/Hill_climbing)
