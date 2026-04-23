# SoftISP Pipeline for libcamera

A production-ready pipeline handler for the SoftISP (Software Image Signal Processor) that enables image processing for both real and virtual cameras.

## Quick Start

```bash
# List cameras (creates virtual camera automatically)
export LIBCAMERA_PIPELINES_MATCH_LIST="SoftISP"
cam --list

# Capture from virtual camera
cam -c softisp_virtual -C 5 -o frame-#.bin
```

## Documentation

- **Full Pipeline Documentation**: See [`README_SOFTISP_PIPELINE.md`](README_SOFTISP_PIPELINE.md)
- **IPA Module**: See [`src/ipa/softisp/README.md`](src/ipa/softisp/README.md)
- **Architecture**: See [`src/ipa/softisp/ARCHITECTURE.md`](src/ipa/softisp/ARCHITECTURE.md)

## Features

✅ Virtual camera support (no hardware required)  
✅ IPA module integration (optional ONNX-based processing)  
✅ Real camera support (ready for V4L2 integration)  
✅ Thread-safe request processing  
✅ Full libcamera integration  

## Building

```bash
# With IPA support (requires ONNX runtime)
meson setup build -Dpipelines=softisp -Dsoftisp=enabled
ninja -C build

# Without IPA (for testing)
meson setup build -Dpipelines=softisp -Dsoftisp=disabled
ninja -C build
```

## Usage

```bash
# List available cameras
export LIBCAMERA_PIPELINES_MATCH_LIST="SoftISP"
cam --list

# Capture frames
cam -c softisp_virtual -C 10 -o capture-#.bin

# Get camera info
cam -c softisp_virtual --info
```

## Key Components

- **PipelineHandlerSoftISP**: Main pipeline handler
- **SoftISPCameraData**: Camera-specific data and processing
- **VirtualCamera**: Virtual camera implementation
- **IPA Module**: Optional image processing (ONNX-based)

## Testing

The pipeline includes comprehensive testing support:
- Virtual camera works without hardware
- IPA module is optional
- Full debug logging available

See the [detailed pipeline documentation](README_SOFTISP_PIPELINE.md) for testing instructions.

## License

LGPL-2.1-or-later (consistent with libcamera)
