# SoftISP IPA - Modular Structure

## Overview
The SoftISP IPA module has been refactored into a modular structure where each method is implemented in a separate file, all located in the same directory (flat structure, no subfolders).

## File Structure
```
src/ipa/softisp/
├── softisp.h              # Class declaration (skeleton header)
├── softisp.cpp            # Skeleton implementation (includes methods)
├── init.cpp               # init() - Load ONNX models
├── start.cpp              # start() - Start processing
├── stop.cpp               # stop() - Stop processing
├── configure.cpp          # configure() - Configure pipeline
├── queueRequest.cpp       # queueRequest() - Queue frame requests
├── computeParams.cpp      # computeParams() - Compute parameters
├── processStats.cpp       # processStats() - Stage 1: Stats → AWB/AE
├── processFrame.cpp       # processFrame() - Stage 2: Bayer → RGB/YUV
├── logPrefix.cpp          # logPrefix() - Logging prefix
├── onnx_engine.cpp        # ONNX Runtime wrapper
├── softisp_module.cpp     # Module entry point (ipaCreate, ipaModuleInfo)
└── README.md              # Main documentation
```

## How It Works

### Skeleton (softisp.cpp)
The main `softisp.cpp` file acts as a skeleton:
1. Opens the `libcamera::ipa::soft` namespace
2. Implements constructor/destructor
3. Includes all method files via `#include "method.cpp"`
4. Closes the namespace

### Method Files
Each method file (e.g., `processStats.cpp`) contains:
1. The method implementation
2. No namespace declarations (they're already open from softisp.cpp)
3. Includes `softisp.h` for class definition

### Example: processStats.cpp
```cpp
/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId,
                           const ControlList &sensorControls)
{
    // Implementation here
    // No namespace needed - already open from softisp.cpp
}
```

## Benefits

### 1. Modularity
- Each method is isolated in its own file
- Easy to find and modify specific functionality
- Clear separation of concerns

### 2. Maintainability
- Small, focused files (easier to read)
- Less cognitive load when editing
- Clear method boundaries

### 3. Testability
- Individual methods can be tested in isolation
- Easy to mock dependencies
- Clear test cases per method

### 4. Collaboration
- Multiple developers can work on different methods simultaneously
- Reduced merge conflicts
- Clear ownership per method

### 5. Flat Structure
- No subdirectories to navigate
- All files in one place
- Simple build configuration

## Architecture: Caller/Callee Pattern

### Caller (Pipeline)
- Pre-calculates buffer sizes
- Maps buffers via `mmap()`
- Prepares all resources
- Owns memory management

### Callee (IPA - Stateless)
- Reads from pre-mapped buffers
- Processes data (no state between calls)
- Writes results back to buffers
- No memory allocation during processing

## Two-Stage Processing

### Stage 1: `processStats()` (processStats.cpp)
- **Input**: Sensor statistics from SharedFD
- **Model**: `algo.onnx`
- **Output**: AWB/AE parameters via `metadataReady` signal
- **Purpose**: Compute Auto White Balance and Auto Exposure

### Stage 2: `processFrame()` (processFrame.cpp)
- **Input**: Bayer frame from SharedFD + AWB/AE parameters
- **Model**: `applier.onnx`
- **Output**: Processed RGB/YUV frame written back to SharedFD
- **Purpose**: Apply ISP pipeline, convert Bayer to RGB/YUV

## Building

The build system (`meson.build`) only compiles:
- `softisp.cpp` (includes all method files)
- `onnx_engine.cpp`
- `softisp_module.cpp`

Individual method files are NOT compiled separately - they're included by `softisp.cpp`.

## Adding New Methods

1. Create `newmethod.cpp` in `src/ipa/softisp/`
2. Implement the method (no namespace needed)
3. Add `#include "newmethod.cpp"` to `softisp.cpp`
4. Declare the method in `softisp.h`

## TODO: ONNX Implementation

Methods marked with TODO comments need ONNX inference logic:
- `processStats.cpp`: Read stats, run algo.onnx, emit metadata
- `processFrame.cpp`: Read Bayer, run applier.onnx, write RGB/YUV

## License
LGPL-2.1-or-later
