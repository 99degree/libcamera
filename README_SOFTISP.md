# SoftISP - ONNX-Based Image Processing Pipeline

## Quick Start

### Prerequisites
- libcamera 0.7.0+
- ONNX Runtime 1.14.0+
- C++20 compiler

### Build
```bash
# Configure with SoftISP pipeline
meson setup softisp_only -Dpipelines=softisp -Dsoftisp=enabled -Ddevelopment=true

# Build
ninja -C softisp_only
```

### Run
```bash
# Set model directory (required for ONNX models)
export SOFTISP_MODEL_DIR=/path/to/models

# List cameras
export LIBCAMERA_PIPELINES_MATCH_LIST="SoftISP"
LD_LIBRARY_PATH=./softisp_only/src/libcamera ./softisp_only/src/apps/cam/cam --list

# Expected output:
# Available cameras:
# 1: (softisp_virtual)
```

## Features

- ✅ **Virtual Camera**: 1920×1080 Bayer (SBGGR10) format
- ✅ **ONNX Integration**: Dual-model pipeline (algo + applier)
- ✅ **MOJOM Interface**: Full libcamera IPA compliance
- ✅ **Development Mode**: Skip signature verification for testing
- ✅ **Validation Script**: Automatic meson.build validation

## Architecture

The SoftISP pipeline consists of:
1. **Pipeline Handler**: Manages virtual camera and request processing
2. **IPA Module**: Performs ONNX-based image processing
3. **OnnxEngine**: Wrapper for ONNX Runtime inference
4. **Virtual Camera**: Generates test frames for development

## Models

The pipeline requires two ONNX models:
- **algo.onnx**: Statistics calculation (AWB/AE)
- **applier.onnx**: Frame processing (ISP pipeline)

Place models in the directory specified by `SOFTISP_MODEL_DIR`.

## Development

### Validation
Always run validation before committing:
```bash
./scripts/validate-meson.sh
```

### Testing
```bash
# Run virtual camera test
./softisp_only/src/tools/softisp-test

# Check logs
export LIBCAMERA_LOG_LEVELS="SoftISPPipeline:Info,SoftIsp:Info"
```

## Documentation

- [Complete Status](SOFTISP_PIPELINE_COMPLETE.md)
- [Pipeline Details](README_SOFTISP_PIPELINE.md)
- [Validation Guide](MESON_BUILD_VALIDATION_GUIDE.md)
- [Architecture](src/ipa/softisp/ARCHITECTURE.md)

## Troubleshooting

### "IPA module not available"
- Ensure `SOFTISP_MODEL_DIR` is set
- Check that models exist in the specified directory
- Verify build with `-Ddevelopment=true`

### "Camera not found"
- Use camera ID 1: `cam -c 1`
- Check `cam --list` for available cameras

### Build errors
- Run validation: `./scripts/validate-meson.sh`
- Clean and rebuild: `ninja -C softisp_only clean`

## License

LGPL-2.1-or-later

---

**Version**: 1.0.0  
**Last Updated**: 2026-04-24  
**Branch**: feature/softisp-pipeline-only
