# CoefficientManager - User Override Layer for SoftISP

## Overview

The `CoefficientManager` is a new layer inserted between `algo.onnx` and `applier.onnx` inference that allows:
- **User overrides** of ISP coefficients (AWB gains, CCM, gamma, etc.)
- **Rule-based modifications** (auto-exposure, color temperature compensation)
- **Configuration persistence** (load/save settings to file)
- **LUT support** for custom tone mapping

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ algo.onnx (4 inputs → 15 outputs)                          │
│ Generates ISP coefficients from image statistics            │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│ CoefficientManager (NEW!)                                   │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ 1. Extract coefficients from algo.onnx outputs         │ │
│ │ 2. Apply user overrides (if set)                       │ │
│ │ 3. Apply rule-based adjustments                        │ │
│ │    - Auto-exposure compensation                        │ │
│ │    - Color temperature compensation                    │ │
│ │ 4. Apply LUT if available                              │ │
│ └─────────────────────────────────────────────────────────┘ │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────┐
│ applier.onnx (10 inputs → 7 outputs)                       │
│ Applies modified coefficients to produce final output       │
└─────────────────────────────────────────────────────────────┘
```

## ISP Coefficients Structure

The `ISPCoefficients` structure holds all ISP parameters:

```cpp
struct ISPCoefficients {
    // AWB gains
    float awbGains[3]; // [R, G, B]
    
    // Color Correction Matrix (3x3)
    float ccm[9];
    
    // Tone mapping curve (16 control points)
    float tonemapCurve[16];
    
    // Gamma value
    float gammaValue;
    
    // RGB to YUV conversion matrix (3x3)
    float rgb2yuvMatrix[9];
    
    // Chroma subscale factor
    float chromaSubsampleScale;
    
    // Image metadata
    int imageWidth;
    int imageHeight;
    int frameId;
    float blackLevelOffset;
    
    // Override flags
    bool overrideAwbGains;
    bool overrideCcm;
    bool overrideTonemap;
    bool overrideGamma;
    bool overrideRgb2yuv;
    bool overrideChroma;
};
```

## API Usage

### Setting Overrides Programmatically

```cpp
#include "src/ipa/softisp/softisp.h"

// Get SoftIsp instance (via IPA interface)
ipa::soft::SoftIsp* isp = ...;

// Set AWB gains
isp->setAwbGains(1.5f, 1.0f, 2.0f);

// Set gamma
isp->setGamma(2.2f);

// Set custom CCM matrix (3x3, row-major)
float ccm[9] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};
isp->setCcm(ccm);

// Set tone mapping curve (16 points)
float tonemap[16] = {0.0f, 0.1f, 0.25f, 0.4f, ...};
isp->setTonemapCurve(tonemap);

// Set RGB to YUV matrix
float rgb2yuv[9] = {
    0.299f, 0.587f, 0.114f,
    -0.169f, -0.331f, 0.5f,
    0.5f, -0.419f, -0.081f
};
isp->setRgb2yuvMatrix(rgb2yuv);

// Set chroma subscale
isp->setChromaSubsampleScale(1.0f);

// Clear all overrides
isp->clearOverrides();
```

### Configuration File

Create a configuration file (e.g., `softisp_config.txt`):

```
# SoftISP Configuration File

# AWB gains (R G B)
awb_gains 1.5 1.0 2.0

# Gamma value
gamma 2.2

# Color temperature (Kelvin)
color_temp 6500

# Enable auto-exposure adjustment
auto_exposure 1
```

Load configuration:

```cpp
isp->loadConfig("/path/to/softisp_config.txt");
```

Save current configuration:

```cpp
isp->saveConfig("/path/to/softisp_config.txt");
```

## Rule-Based Adjustments

### 1. Color Temperature Compensation

When enabled, adjusts AWB gains based on target color temperature:

```cpp
// Enable in config file:
color_temp 6500

// Or programmatically (via future API):
// isp->setColorTemperature(6500.0f);
```

### 2. Auto-Exposure Adjustment

Automatically adjusts gamma based on scene brightness:

```cpp
# Enable in config file:
auto_exposure 1
```

## LUT Support

Custom Look-Up Tables can be applied to the tone mapping curve:

```cpp
// Create 256-point LUT
std::vector<float> lut(256);
for (int i = 0; i < 256; i++) {
    lut[i] = powf(i / 255.0f, 2.2f) * 255.0f;
}

// Apply LUT (future API)
// isp->setRgbLut(lut.data(), lut.size());
```

## Processing Flow

For each frame:

1. **algo.onnx inference** → Generates 15 coefficient outputs
2. **Extract coefficients** → Populate `ISPCoefficients` structure
3. **Apply user overrides** → Replace specific coefficients if set
4. **Apply rules** → Auto-exposure, color temperature adjustments
5. **Apply LUT** → If custom LUT is loaded
6. **applier.onnx inference** → Uses modified coefficients

## Example: Custom White Balance

```cpp
// Set custom white balance
isp->setAwbGains(1.8f, 1.0f, 1.2f);

// Run capture
// The CoefficientManager will apply these gains
// instead of the model's default AWB gains
```

## Example: Film Look

```cpp
// Set film-like gamma
isp->setGamma(2.4f);

// Set custom tone mapping curve for film look
float filmTonemap[16] = {
    0.0f, 0.05f, 0.12f, 0.22f,
    0.35f, 0.50f, 0.65f, 0.78f,
    0.88f, 0.94f, 0.97f, 0.985f,
    0.992f, 0.996f, 0.998f, 1.0f
};
isp->setTonemapCurve(filmTonemap);
```

## Files Modified/Created

- `src/ipa/softisp/softisp.h` - Added `ISPCoefficients` and `CoefficientManager` classes
- `src/ipa/softisp/softisp.cpp` - Integrated CoefficientManager into inference pipeline
- `src/ipa/softisp/coefficient_manager.cpp` - New file with CoefficientManager implementation
- `src/ipa/softisp/meson.build` - Added `coefficient_manager.cpp` to build

## Benefits

1. **Flexibility**: Users can override any ISP parameter without retraining models
2. **Tuning**: Fine-tune image quality for specific scenes or preferences
3. **Creative Control**: Apply custom looks (film, B&W, etc.)
4. **Adaptation**: Compensate for lighting conditions automatically
5. **Persistence**: Save/load configurations for consistent results

## Future Enhancements

- [ ] Add more rule-based adjustments (noise reduction, sharpening)
- [ ] Support for per-frame coefficient interpolation
- [ ] Real-time coefficient adjustment via ControlList
- [ ] Multiple preset configurations (portrait, landscape, night, etc.)
- [ ] GPU-accelerated LUT application
