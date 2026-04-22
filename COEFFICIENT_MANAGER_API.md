# CoefficientManager API Reference

## Public API (User-Accessible)

### 1. Override Setting Methods
These methods allow users to override specific ISP coefficients:

```cpp
// Set AWB (Auto White Balance) gains
void setAwbGains(float r, float g, float b);
// Parameters:
//   r - Red gain (e.g., 1.5f)
//   g - Green gain (e.g., 1.0f)
//   b - Blue gain (e.g., 2.0f)

// Set Color Correction Matrix (3x3, row-major)
void setCcm(const float* matrix);
// Parameters:
//   matrix - Pointer to 9 float values (3x3 matrix)
//   Example: {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f}

// Set Tone Mapping Curve (16 control points)
void setTonemapCurve(const float* curve);
// Parameters:
//   curve - Pointer to 16 float values (0.0 to 1.0 range)

// Set Gamma value
void setGamma(float value);
// Parameters:
//   value - Gamma exponent (e.g., 2.2f for sRGB)

// Set RGB to YUV conversion matrix (3x3, row-major)
void setRgb2yuvMatrix(const float* matrix);
// Parameters:
//   matrix - Pointer to 9 float values (3x3 matrix)

// Set Chroma Subsample Scale
void setChromaSubsampleScale(float scale);
// Parameters:
//   scale - Chroma scaling factor (e.g., 1.0f)

// Clear all user overrides (use model outputs directly)
void clearOverrides();
```

### 2. Configuration Methods
Load/save settings to/from file:

```cpp
// Load configuration from file
int loadConfig(const std::string& configPath);
// Returns: 0 on success, negative error code on failure
// File format:
//   awb_gains 1.5 1.0 2.0
//   gamma 2.2
//   color_temp 6500
//   auto_exposure 1

// Save current configuration to file
int saveConfig(const std::string& configPath) const;
// Returns: 0 on success, negative error code on failure
```

### 3. Query Methods
```cpp
// Get current coefficient state (after all overrides/rules applied)
const ISPCoefficients& getCurrentCoefficients() const;
// Returns: Reference to current ISPCoefficients structure
```

### 4. SoftIsp Public API (Delegates to CoefficientManager)
These methods are exposed via the SoftIsp class:

```cpp
// All override setters (same as above)
void setAwbGains(float r, float g, float b);
void setCcm(const float* matrix);
void setGamma(float value);
void setTonemapCurve(const float* curve);
void setRgb2yuvMatrix(const float* matrix);
void setChromaSubsampleScale(float scale);
void clearOverrides();

// Configuration
int loadConfig(const std::string& configPath);
int saveConfig(const std::string& configPath) const;

// Query
const ISPCoefficients& getCurrentCoefficients() const;
```

## Internal API (Private Methods)

These are called automatically during processing:

```cpp
// Apply all rules and modifications (called per frame)
void applyRules(ISPCoefficients* coeffs);

// Individual rule applications (called by applyRules)
void applyUserOverrides(ISPCoefficients* coeffs);
void applyColorTemperatureCompensation(ISPCoefficients* coeffs);
void applyAutoExposureAdjustment(ISPCoefficients* coeffs);
```

## ISPCoefficients Structure

The data structure holding all ISP parameters:

```cpp
struct ISPCoefficients {
    float awbGains[3];          // [R, G, B]
    float ccm[9];               // 3x3 Color Correction Matrix
    float tonemapCurve[16];     // 16 tone mapping control points
    float gammaValue;           // Gamma exponent
    float rgb2yuvMatrix[9];     // 3x3 RGB to YUV matrix
    float chromaSubsampleScale; // Chroma scaling factor
    int imageWidth;             // Image width
    int imageHeight;            // Image height
    int frameId;                // Frame counter
    float blackLevelOffset;     // Black level
    bool overrideAwbGains;      // Flag: AWB overridden?
    bool overrideCcm;           // Flag: CCM overridden?
    bool overrideTonemap;       // Flag: Tonemap overridden?
    bool overrideGamma;         // Flag: Gamma overridden?
    bool overrideRgb2yuv;       // Flag: RGB2YUV overridden?
    bool overrideChroma;        // Flag: Chroma overridden?
};
```

## Usage Example

```cpp
#include "src/ipa/softisp/softisp.h"

// Get SoftIsp instance
ipa::soft::SoftIsp* isp = ...;

// Set custom white balance
isp->setAwbGains(1.8f, 1.0f, 1.2f);

// Set film-like gamma
isp->setGamma(2.4f);

// Load additional settings from file
isp->loadConfig("/path/to/config.txt");

// Process frame (overrides applied automatically)
isp->processStats(frame, bufferId, controls);

// Query final coefficients
const ISPCoefficients& coeffs = isp->getCurrentCoefficients();
printf("AWB Gains: R=%.2f G=%.2f B=%.2f\n", 
       coeffs.awbGains[0], coeffs.awbGains[1], coeffs.awbGains[2]);
```

## Configuration File Format

```txt
# Comments start with #
awb_gains 1.5 1.0 2.0
gamma 2.2
color_temp 6500
auto_exposure 1
```

## Return Values

- `0`: Success
- `-ENOENT`: File not found (loadConfig)
- `-EIO`: I/O error (saveConfig)
- Negative values: Other errors
