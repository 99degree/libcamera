# SoftISP Architecture

## Overview
SoftISP is an IPA (Image Processing Algorithm) module for libcamera that uses two ONNX models to perform ISP processing:
- `algo.onnx`: Generates ISP coefficients from statistics (decision logic)
- `applier.onnx`: Applies coefficients to produce final gains (execution logic)

## Architecture Layers

### 1. Algorithm Layer (`softisp.h/cpp`)
- Implements `libcamera::ipa::Algorithm<T>` template class
- Contains the ONNX inference logic
- Processes frame statistics and outputs ISP coefficients
- Registered with `REGISTER_ALGORITHM(SoftIsp)` macro
- **Purpose**: "What coefficients to use?" (decision logic)

### 2. Module Layer (`softisp_module.h/cpp`)
- Implements `ipa::soft::IPASoftInterface`
- Exports `ipaCreate()` function
- Defines `ipaModuleInfo` with:
  - `module.name = "softisp"` (module identifier)
  - `module.pipelineName = "simple"` (matches "simple" pipeline handler)
- Wraps the SoftISP algorithm and provides IPA interface
- **Purpose**: "How to apply coefficients?" (execution logic)

## How It Works

```
┌─────────────────────────────────────────────────────────────┐
│ Application (libcamera-vid, etc.)                           │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│ Pipeline Handler: "simple" (src/libcamera/pipeline/simple) │
│ - Configures camera and buffers                              │
│ - Calls IPAManager::createIPA() to load IPA module          │
└─────────────────────────┬───────────────────────────────────┘
                          │
                          │ IPAManager matches module by:
                          │   - pipelineName == "simple"
                          │   - pipelineVersion in range
                          ▼
┌─────────────────────────────────────────────────────────────┐
│ IPA Module: ipa_softisp.so                                  │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ SoftISPModule (implements IPASoftInterface)             │ │
│ │ - Manages frame lifecycle                                │ │
│ │ - Delegates to SoftIsp algorithm                         │ │
│ │ - Returns ControlList with AWB gains                     │ │
│ └─────────────────────┬───────────────────────────────────┘ │
│                       │                                     │
│                       ▼                                     │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ SoftIsp Algorithm (implements Algorithm<T>)             │ │
│ │ - Runs algo.onnx: statistics → coefficients             │ │
│ │ - Runs applier.onnx: coefficients → gains               │ │
│ │ - Extracts AWB gains (R, G, B)                          │ │
│ └─────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
                          │
                          │ Returns metadata with colourGains
                          ▼
┌─────────────────────────────────────────────────────────────┐
│ Pipeline Handler applies gains to frame                     │
│ Returns processed frame to application                      │
└─────────────────────────────────────────────────────────────┘
```

## Key Design Decisions

### Why Two Layers?
- **Algorithm Layer**: Pure inference logic, easy to test independently
- **Module Layer**: IPA interface, frame lifecycle management
- **Separation of Concerns**: Algorithm can be swapped without changing module

### Why `pipelineName = "simple"`?
- The "simple" pipeline handler loads IPA modules by matching `pipelineName`
- Setting `pipelineName = "simple"` allows our module to be loaded by the simple pipeline
- This makes SoftISP a drop-in replacement for `ipa_simple.so`

### How It Compares to VC4
| Component | VC4 (Broadcom) | SoftISP |
|-----------|----------------|---------|
| **Pipeline Handler** | `RpiHandler` (VC4 hardware) | `SimpleHandler` (software) |
| **IPA Module** | `ipa_rpi.so` | `ipa_softisp.so` |
| **Algorithms** | `AwbAlgorithm`, `AgcAlgorithm`, etc. | `SoftIsp` (single AI algorithm) |
| **Processing** | Hardware ISP + classical algorithms | Software AI-based ISP |
| **Interface** | `IPARPiInterface` | `IPASoftInterface` |

## Configuration

### Model Location
- Environment variable: `SOFTISP_MODEL_DIR`
- Default: `/usr/share/libcamera/ipa/softisp/`
- Required files:
  - `algo.onnx` (25KB)
  - `applier.onnx` (20KB)

### Build Options
```bash
meson setup build -Dsoftisp=enabled -Dpipelines=simple
meson compile -C build
```

### Testing
```bash
# Model validation test
./build/src/ipa/softisp/softisp_test

# Full inference test
./build/src/ipa/softisp/softisp_full_test

# Real camera test (once build issues are resolved)
export SOFTISP_MODEL_DIR=/path/to/models
libcamera-vid --pipelines simple --timeout 5000 -o test.264
```

## File Structure
```
src/ipa/softisp/
├── softisp.h              # Algorithm class declaration
├── softisp.cpp            # Algorithm implementation (ONNX inference)
├── softisp_module.h       # Module class declaration
├── softisp_module.cpp     # Module implementation (IPA interface)
├── softisp_test.cpp       # Standalone model loading test
├── softisp_full_test.cpp  # Full inference test
├── meson.build            # Build configuration
├── README_TESTING.md      # Testing guide
└── ARCHITECTURE.md        # This file
```

## Implementation Status

### ✅ Completed
- Algorithm layer (ONNX inference logic)
- Module layer (IPA interface wrapper)
- Test programs (standalone and IPA module tests)
- Build configuration
- Documentation

### ⚠️ Pending
- Full libcamera build (blocked by Termux-specific issues)
- Real camera testing (requires successful build)
- Performance optimization

## Troubleshooting

### Module Not Loading
- Verify `pipelineName` matches pipeline handler name ("simple")
- Check `ipaModuleInfo` is correctly defined
- Ensure module is in the correct directory (`$IPA_MODULE_DIR`)

### Model Loading Fails
- Check `SOFTISP_MODEL_DIR` environment variable
- Verify model files exist and are not corrupted
- Check ONNX Runtime version compatibility

### Inference Errors
- Verify input tensor shapes match model expectations
- Check that statistics are properly extracted from frames
- Review ONNX Runtime error messages

## References
- libcamera IPA documentation: `docs/ipa.md`
- Simple pipeline source: `src/libcamera/pipeline/simple/`
- VC4 pipeline source: `src/libcamera/pipeline/rpi/vc4/`
- Algorithm interface: `src/ipa/libipa/algorithm.h`
- Soft IPA interface: `build/include/libcamera/ipa/soft_ipa_interface.h`
