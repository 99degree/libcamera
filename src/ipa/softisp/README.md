# SoftISP IPA Module

A Software-based Image Signal Processor (ISP) implementation for libcamera using ONNX machine learning models.

## Overview

SoftISP is a custom IPA (Image Processing Algorithm) module that implements ISP functionality using two ONNX models:
- `algo.onnx` - Generates ISP coefficients from frame statistics
- `applier.onnx` - Applies coefficients to produce final image metadata

## Build Requirements

- libcamera development files
- ONNX Runtime (for runtime inference)
- C++20 compiler
- Meson build system

## Building

```bash
# Configure with SoftISP enabled
meson setup build \
  -Dsoftisp=enabled \
  -Dpipelines='softisp,dummysoftisp'

# Build
meson compile -C build
```

This produces:
- `libcamera.so` - Main library with SoftISP pipelines
- `ipa_softisp.so` - IPA module for real cameras
- `ipa_softisp_virtual.so` - IPA module for dummy cameras

## Usage

### Environment Variables

- `SOFTISP_MODEL_DIR` - Path to directory containing ONNX models

### Pipelines

1. **softisp** - For real V4L2 cameras
2. **dummysoftisp** - For testing without hardware

### Example

```bash
# With real camera
libcamera-vid --pipeline softisp \
  --output video.yuv \
  --width 1920 --height 1080

# With dummy camera (testing)
libcamera-vid --pipeline dummysoftisp \
  --output test.yuv \
  --width 1920 --height 1080
```

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    libcamera Core                        │
├─────────────────────────────────────────────────────────┤
│                                                          │
│  ┌──────────────┐    ┌──────────────┐                   │
│  │   Pipeline   │───▶│     IPA      │                   │
│  │   Handler    │    │   Module     │                   │
│  └──────────────┘    └──────────────┘                   │
│         │                   │                            │
│         ▼                   ▼                            │
│  ┌──────────────┐    ┌──────────────┐                   │
│  │  CameraData  │    │   SoftIsp    │                   │
│  │              │    │ (Algorithm)  │                   │
│  └──────────────┘    └──────────────┘                   │
│                            │                             │
│                            ▼                             │
│                    ┌──────────────┐                      │
│                    │  ONNX Models │                      │
│                    │ algo.onnx    │                      │
│                    │ applier.onnx │                      │
│                    └──────────────┘                      │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

## File Structure

```
src/ipa/softisp/
├── README.md              # This file
├── algorithm.h            # Algorithm type alias
├── module.h               # Module type definition
├── softisp.h              # SoftIsp class declaration
├── softisp.cpp            # SoftIsp implementation
├── softisp_module.cpp     # IPA module for "softisp" pipeline
└── softisp_virtual_module.cpp  # IPA module for "dummysoftisp" pipeline
```

## ONNX Model Specifications

### algo.onnx

**Inputs (4):**
1. Frame statistics (brightness, contrast, etc.)
2. Scene type indicators
3. Exposure parameters
4. White balance data

**Outputs (15):**
1-10: ISP coefficient values
11-15: Metadata parameters

### applier.onnx

**Inputs (10):**
1-7: Raw frame data
8-10: Coefficients from algo.onnx

**Outputs (7):**
1-7: Processed metadata for camera

## Development

See the following documentation files for more information:
- `SOFTISP_BUILD_SKILLS.md` - Troubleshooting guide
- `SOFTISP_TODO.md` - Implementation tasks
- `SOFTISP_SUMMARY.md` - Project summary

## License

LGPL-2.1-or-later

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## Support

For issues and questions, please open an issue on the project tracker.
