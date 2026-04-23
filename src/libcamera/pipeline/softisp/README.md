# DummySoftISP Pipeline with Virtual Camera

## Overview

The DummySoftISP pipeline now includes a **standalone Virtual Camera** class that provides test pattern generation for testing the SoftISP pipeline without requiring real camera hardware.

## Architecture

### Virtual Camera Class (`virtual_camera.h/cpp`)

The `VirtualCamera` class is a **standalone, reusable component** that:

- Generates various test patterns (solid color, grayscale, color bars, checkerboard, sine wave)
- Runs in a separate thread for asynchronous frame generation
- Provides a simple API similar to standard camera interfaces
- Can be used independently or integrated into any pipeline

### Key Features

1. **Pattern Generation**: Multiple test patterns for different testing scenarios
2. **Threaded Operation**: Runs in background thread, non-blocking
3. **Configurable**: Brightness, contrast, and pattern type can be adjusted
4. **Buffer Queue**: Manages buffer queue for frame production
5. **Standard API**: Similar interface to real camera devices

### Usage

```cpp
// Create virtual camera instance
auto virtualCam = std::make_unique<VirtualCamera>();

// Initialize with resolution and format
virtualCam->init(1920, 1080, formats::UYVY888);

// Set pattern type
virtualCam->setPattern(VirtualCamera::Pattern::ColorBars);

// Start generation
virtualCam->start();

// Queue buffers for processing
virtualCam->queueBuffer(buffer);

// Stop when done
virtualCam->stop();
```

## Integration with DummySoftISP

The DummySoftISP pipeline now:

1. **Creates a VirtualCamera instance** during camera initialization
2. **Uses it for frame generation** when no real camera is available
3. **Automatically detects** real cameras and falls back to virtual if none found
4. **Maintains compatibility** with the standard camera API

### Automatic Detection

The `match()` method now:
- Enumerates media devices to find real cameras
- If real camera found: Does NOT register virtual camera
- If no real camera: Registers as virtual camera for testing

## Test Patterns

| Pattern | Description | Use Case |
|---------|-------------|----------|
| SolidColor | Uniform brightness | White balance testing |
| Grayscale | Gradient from black to white | Exposure testing |
| ColorBars | SMPTE color bars | Color accuracy testing |
| Checkerboard | Black/white checkerboard | Focus testing |
| SineWave | Sinusoidal wave pattern | Resolution testing |

## Building

```bash
meson setup build -Dpipelines='dummysoftisp'
meson compile -C build
```

The build now includes both `softisp.cpp` and `virtual_camera.cpp`.

## Files

- `virtual_camera.h` - Virtual camera class declaration
- `virtual_camera.cpp` - Virtual camera implementation
- `softisp.h` - Pipeline handler declaration (updated)
- `softisp.cpp` - Pipeline handler implementation (updated)
- `meson.build` - Build configuration (updated)

## Future Enhancements

1. Add more test patterns (noise, edges, etc.)
2. Support for video file playback
3. Network stream input
4. Real-time pattern modification via controls
5. Synchronization with real camera timestamps
