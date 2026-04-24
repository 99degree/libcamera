# SoftISP Pipeline Handler

## Overview
The SoftISP pipeline handler implements a virtual camera for testing and development of ONNX-based image processing algorithms.

## Components

### PipelineHandlerSoftISP
- Manages virtual camera lifecycle
- Handles request processing
- Integrates with IPA module

### VirtualCamera
- Generates synthetic Bayer frames (1920×1080)
- Supports SBGGR10 format
- Simulates real camera behavior

### SoftISPCameraData
- Camera session data
- Buffer management
- Thread-safe operations

## Usage

The pipeline is automatically loaded when:
1. Built with `-Dpipelines=softisp`
2. `LIBCAMERA_PIPELINES_MATCH_LIST="SoftISP"` is set

## Configuration

### Environment Variables
- `SOFTISP_MODEL_DIR`: Path to ONNX models (required)

### Log Levels
```bash
export LIBCAMERA_LOG_LEVELS="SoftISPPipeline:Info,SoftIsp:Debug"
```

## Implementation Notes

### generateConfiguration()
- Returns 1920×1080 SBGGR10 stream
- Validates configuration
- Sets buffer count to 4

### processRequest()
- Queues requests to virtual camera
- Calls IPA module if available
- Completes requests with metadata

## Testing

```bash
# List cameras
cam --list

# Capture frame
cam -c 1 --capture=1 --file=test.bin
```

## Files

- `softisp.cpp` - Main pipeline implementation
- `softisp.h` - Pipeline header
- `virtual_camera.cpp` - Virtual camera implementation
- `virtual_camera.h` - Virtual camera header

## License

LGPL-2.1-or-later
