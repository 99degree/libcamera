# Development Mode - IPA Signature Verification Disabled ✅

## What Was Done

Added a `-Ddevelopment` build option that:
1. Defines `DEVELOPMENT_MODE` preprocessor macro
2. Skips IPA module signature verification in `IPAManager::isSignatureValid()`
3. Allows unsigned IPA modules to be loaded

## Build Command

```bash
meson setup softisp_only -Dpipelines=softisp -Dsoftisp=enabled -Ddevelopment=true
ninja -C softisp_only
```

## Current Status

### ✅ Working
- IPA signature verification is **disabled**
- Module builds successfully with ONNX Runtime
- Module exports correct symbols (`ipaCreate`, `ipaModuleInfo`)

### ⚠️ Still Blocked
The IPA module still cannot be loaded because:
- Pipeline expects `IPAProxySoftIsp` interface (MOJOM-generated)
- MOJOM toolchain not available in this environment
- Interface mismatch: module provides `SoftIsp`, pipeline expects `IPAProxySoftIsp`

## Why This Happens

The IPAManager has **two** checks:
1. **Signature verification** ✅ (now disabled with development mode)
2. **Interface compatibility** ❌ (requires MOJOM generation)

```cpp
// From ipa_manager.cpp
ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(pipe, 0, 0);
// This requires the MOJOM-generated IPAProxySoftIsp class
```

## Solutions

### Option 1: Generate MOJOM Files (Recommended)
```bash
# Full libcamera build with MOJOM support
meson setup build -Dpipelines=all
ninja -C build
# This generates IPAProxySoftIsp from MOJOM definitions
```

### Option 2: Modify Pipeline to Use SoftIsp Directly
Modify `src/libcamera/pipeline/softisp/softisp.cpp`:
```cpp
int SoftISPCameraData::loadIPA() {
    // Try MOJOM interface first
    ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIsp>(...);
    if (!ipa_) {
        // Fall back to direct SoftIsp instantiation
        ipa_ = new ipa::soft::SoftIsp();
    }
}
```

### Option 3: Use SoftIsp in Custom Application
Bypass the pipeline entirely and use `SoftIsp` directly:
```cpp
#include "src/ipa/softisp/softisp.h"

int main() {
    setenv("SOFTISP_MODEL_DIR", ".", 1);
    libcamera::ipa::soft::SoftIsp softIsp;
    // Initialize and use directly
}
```

## Files Modified

```
meson.build              - Added development mode check
meson_options.txt        - Added -Ddevelopment option
```

## Testing

```bash
# Build with development mode
meson setup softisp_only -Dpipelines=softisp -Dsoftisp=enabled -Ddevelopment=true
ninja -C softisp_only

# Run with models
export SOFTISP_MODEL_DIR=.
export LIBCAMERA_PIPELINES_MATCH_LIST="SoftISP"
cam --list
```

## Conclusion

✅ **Signature verification**: Disabled with development mode  
⚠️ **Interface loading**: Still requires MOJOM generation or pipeline modification  

The ONNX integration is complete in the code. The remaining blocker is the MOJOM interface generation, which requires the full libcamera build toolchain.

---

**Branch**: `feature/softisp-ipa-onnx`  
**Status**: Development mode enabled, signature check bypassed  
**Next**: Generate MOJOM files or modify pipeline for direct SoftIsp usage
