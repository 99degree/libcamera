# SoftISP Pipeline Usage

## Overview
The SoftISP pipeline is designed to work with **real camera devices** that have been enumerated by the system. It provides SoftISP (Software Image Signal Processor) functionality for image processing.

## Virtual Camera Support
The SoftISP pipeline includes a **virtual camera fallback** that creates a virtual camera when:
1. A media device is successfully enumerated
2. No real cameras are found on that device

**Important**: The virtual camera fallback only works when a media device exists and is successfully enumerated. It does **not** create a media device itself.

## Usage

### With Real Hardware
When running on a device with a real camera:
```bash
export LIBCAMERA_PIPELINES_MATCH_LIST="softisp"
cam -l  # Should list the SoftISP-processed camera
```

### For Testing Without Hardware
For testing without real hardware, use the **Virtual Pipeline** instead:
```bash
export LIBCAMERA_PIPELINES_MATCH_LIST="virtual"
cam -l  # Lists virtual cameras from virtual.yaml configuration
```

The Virtual Pipeline is specifically designed for testing and development without hardware.

## Configuration
The SoftISP pipeline can be configured via IPA modules. See the IPA documentation for details.

## Building
```bash
meson setup build -Dpipelines=softisp -Dsoftisp=enabled
ninja -C build
```

Note: The IPA module requires ONNX runtime for full functionality. For testing without ONNX, build with `-Dsoftisp=disabled`.
