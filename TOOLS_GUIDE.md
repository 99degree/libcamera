# SoftISP Tool Suite Guide

## Overview

The SoftISP tool suite includes two main utilities:

1. **softisp-save** - Advanced frame capture utility (like `cam` app)
2. **softisp-tool** - ONNX model inspection and testing suite

---

## softisp-save: Advanced Frame Capture

### Features

- **Multiple Output Formats**:
  - `raw` - Raw Bayer10 data (default)
  - `yuv` - YUV420/YUV422 format
  - `rgb` - RGB888 format
  - `ppm` - Portable Pixmap image format

- **Metadata Support**: Save camera controls and metadata to `.meta` files

- **Flexible Output Naming**: Use `#` placeholder for frame numbers

- **Custom Resolution**: Configure width and height

- **Batch Capture**: Capture multiple frames in one run

### Usage

```bash
# Basic capture (RAW format)
./build/softisp-save -c softisp_virtual -n 5

# Capture in YUV format with metadata
./build/softisp-save -c softisp_virtual -f yuv -n 3 --metadata -o capture-#

# Capture in RGB format with custom resolution
./build/softisp-save -c softisp_virtual -f rgb -w 1280 -H 720 -n 10 -o frame-#.raw

# Capture as PPM images
./build/softisp-save -c softisp_virtual -f ppm -n 5 -o image-#

# Verbose mode
./build/softisp-save -c softisp_virtual -v -n 1 -o test.raw
```

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `-c, --camera <id>` | Camera ID | `softisp_virtual` |
| `-o, --output <file>` | Output file pattern (use # for frame number) | `frame-#.raw` |
| `-f, --format <fmt>` | Output format: `raw`, `yuv`, `rgb`, `ppm` | `raw` |
| `-n, --frames <num>` | Number of frames to capture | `1` |
| `-w, --width <w>` | Image width | `1920` |
| `-H, --height <h>` | Image height | `1080` |
| `-m, --metadata` | Save metadata to .meta files | Disabled |
| `-v, --verbose` | Verbose output | Disabled |
| `-h, --help` | Show help message | - |

### Output Examples

#### RAW Format (SBGGR10)
```bash
./build/softisp-save -f raw -n 3 -o frame-#
```
Output:
- `frame-0000.raw` (4147200 bytes for 1920x1080)
- `frame-0001.raw`
- `frame-0002.raw`

#### YUV Format with Metadata
```bash
./build/softisp-save -f yuv -n 2 --metadata -o capture-#
```
Output:
- `capture-0000.raw` (3110400 bytes for 1920x1080 YUV420)
- `capture-0000.meta`
- `capture-0001.raw`
- `capture-0001.meta`

#### PPM Format
```bash
./build/softisp-save -f ppm -n 1 -o output
```
Output:
- `output` (PPM image file, viewable in any image viewer)

### Metadata File Format

When `--metadata` is enabled, a `.meta` file is created for each frame:

```
Frame: 0
Timestamp: 1713950400
Controls:
  SensorTimestamp: 123456789
  ExposureTime: 10000
  DigitalGain: 2.0
  ...
```

---

## softisp-tool: ONNX Model Testing Suite

### Features

- Inspect ONNX model structure
- Check tensor shapes and types
- Run inference tests
- Validate coefficient tensors
- Test coefficient applier

### Usage

```bash
# Inspect model structure
./build/src/tools/softisp-tool inspect algo.onnx

# Check tensor shapes
./build/src/tools/softisp-tool shapes algo.onnx

# Display tensor types
./build/src/tools/softisp-tool types algo.onnx

# Comprehensive model info
./build/src/tools/softisp-tool all-info algo.onnx

# Run inference test
./build/src/tools/softisp-tool inference algo.onnx

# Check coefficient tensors
./build/src/tools/softisp-tool check-coeffs algo.onnx
```

### Commands

| Command | Description |
|---------|-------------|
| `inspect` | Display model input/output structure |
| `shapes` | Check tensor dimensions |
| `types` | Display data types |
| `all-info` | Comprehensive model information |
| `test` | Basic ONNX model test |
| `inference` | Run inference with dummy data |
| `check-coeffs` | Validate coefficient tensors |
| `applier-run` | Test coefficient applier |

---

## Building the Tools

### Build Requirements

```bash
# Install dependencies
pkg install clang ninja meson python3 libyaml python-pip python-onnxruntime libevent

# Clone and setup
git clone https://github.com/99degree/libcamera
cd libcamera
git checkout softisp_new

# Configure and build
meson setup build -Dpipelines=softisp -Dsoftisp=enabled -Dcam=enabled -Ddevelopment=true
meson compile -C build
```

### Building softisp-save

```bash
# The tool is built as part of the main build
# After compilation, it will be available at:
./build/softisp-save
```

---

## Troubleshooting

### "Camera not found" Error

**Cause**: The SoftISP virtual camera pipeline is not running.

**Solution**: 
- On real hardware (Raspberry Pi/Rockchip): The pipeline should auto-register
- On Termux/Android: The virtual camera may not work due to lack of V4L2 support

### "Failed to generate configuration" Error

**Cause**: Pipeline handler not responding correctly.

**Solution**: 
- Check that the SoftISP pipeline is built and loaded
- Verify IPA module is in the correct location
- Check `LIBCAMERA_IPA` environment variable

### Metadata Not Saved

**Cause**: `--metadata` flag not used or controls not available.

**Solution**: 
- Add `--metadata` flag to command
- Ensure camera is properly configured

---

## Examples

### Complete Capture Session

```bash
# 1. Test ONNX models first
./build/src/tools/softisp-tool inspect algo.onnx
./build/src/tools/softisp-tool inspect applier.onnx

# 2. Capture frames in different formats
./build/softisp-save -f raw -n 1 -o test-raw.raw
./build/softisp-save -f yuv -n 1 --metadata -o test-yuv
./build/softisp-save -f rgb -n 1 -o test-rgb.raw
./build/softisp-save -f ppm -n 1 -o test-image

# 3. View results
# RAW: Use image viewer that supports raw Bayer
# YUV: Convert with ffmpeg or similar
# RGB: Convert to PNG/JPG
# PPM: View directly in any image viewer
```

### Batch Processing

```bash
# Capture 100 frames in YUV format with metadata
for i in {1..100}; do
  ./build/softisp-save -f yuv -n 1 --metadata -o batch-$(printf "%03d" $i)
done
```

---

## Author

**George Chan** <gchan9527@gmail.com>

SoftISP Pipeline Implementation
libcamera branch: `softisp_new`
