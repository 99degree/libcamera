# SoftISP Build Skills & Troubleshooting Guide

This document captures key findings, patterns, and solutions encountered while building the SoftISP IPA module and pipelines for libcamera on Termux.

---

## 1. Pipeline Registration Macro Placement

**Finding:** The `REGISTER_PIPELINE_HANDLER` macro **must** be placed **outside** the `namespace libcamera` block, but **before** the final closing brace of the file.

**Correct Pattern:**
```cpp
namespace libcamera {
    // ... class definitions and implementations ...
} /* namespace libcamera */

REGISTER_PIPELINE_HANDLER(ClassName, "pipeline-name")
```

**Why:** The macro definition in `include/libcamera/internal/pipeline_handler.h` includes the closing brace:
```cpp
#define REGISTER_PIPELINE_HANDLER(handler, name) \
    static PipelineHandlerFactory<handler> global_##handler##Factory(name); \
} /* namespace libcamera */
```

**Common Mistake:** Placing the macro inside the namespace or adding an extra closing brace after it.

---

## 2. IPA Module Registration

**Finding:** For custom IPA modules that implement the generated interface (like `IPASoftInterface`), use the `ipaCreate()` function pattern, **not** `REGISTER_IPA_ALGORITHM`.

**Correct Pattern (from `rkisp1`, `ipu3`):**
```cpp
// In the .cpp file, outside the namespace:
extern "C" IPAInterface *ipaCreate() {
    return new ipa::soft::SoftIsp();
}
```

**Why:** The `REGISTER_IPA_ALGORITHM` macro is for `Algorithm` subclasses used in the `simple` pipeline's configurable algorithm framework. Direct interface implementations (like `rkisp1`, `vimc`, `ipu3`) use `ipaCreate()`.

---

## 3. Request Completion

**Finding:** Requests are completed via the **PipelineHandler**, not directly on the `Request` object.

**Correct Pattern:**
```cpp
// In PipelineHandler methods:
completeRequest(request);

// In CameraData methods (if needed):
pipe()->completeRequest(request);
```

**Common Mistake:** Calling `request->complete()` directly. The `Request::complete()` method is internal and should not be called directly from pipeline code.

---

## 4. IPA Algorithm Interface

**Finding:** The `IPASoftInterface` (generated from `soft.mojom`) defines the exact virtual methods to implement. Only override the **pure virtual** methods.

**Pure Virtual Methods in `IPASoftInterface`:**
- `init()`
- `start()`
- `stop()`
- `configure()`
- `queueRequest()`
- `computeParams()`
- `processStats()`

**Non-Virtual Methods (do NOT mark as `override`):**
- `mapBuffers()` (has default implementation)
- `unmapBuffers()` (has default implementation)

**Correct Class Definition:**
```cpp
class SoftIsp : public IPASoftInterface, public Module {
public:
    int32_t init(...) override;
    int32_t start() override;
    void stop() override;
    // ... only pure virtual methods ...
    
protected:
    std::string logPrefix() const override;
};
```

---

## 5. Log Category Definition

**Finding:** `LOG_DEFINE_CATEGORY` must be placed **inside** the `namespace libcamera` block, but **outside** any nested namespaces (like `ipa::soft`).

**Correct Pattern:**
```cpp
namespace libcamera {
    LOG_DEFINE_CATEGORY(SoftIsp)
    
    namespace ipa::soft {
        // ... class definitions ...
    }
}
```

**Usage:**
```cpp
LOG(SoftIsp, Info) << "Message";
```

---

## 6. Thread Handling in CameraData

**Finding:** The `Thread` class (from `libcamera/base/thread.h`) uses `exit()` and `wait()` for termination, **not** `stop()`.

**Correct Pattern:**
```cpp
// In destructor:
running_ = false;
exit(0);
wait();

// In stopDevice():
data->running_ = false;
data->exit(0);
data->wait();
```

**Common Mistake:** Calling `Thread::stop()` which doesn't exist.

---

## 7. Mojom-Generated Headers

**Finding:** The `soft.mojom` file must be added to the `pipeline_ipa_mojom_mapping` in `include/libcamera/ipa/meson.build` to generate the interface headers.

**Correct Entry:**
```python
pipeline_ipa_mojom_mapping = {
    # ... other pipelines ...
    'softisp': 'soft.mojom',
    'dummysoftisp': 'soft.mojom',
}
```

**Why:** Without this, the `soft_ipa_interface.h` and `soft_ipa_proxy.h` files are not generated, causing "file not found" errors.

---

## 8. Pipeline Name Restrictions

**Finding:** Pipeline names **cannot contain hyphens (`-`)** because the `REGISTER_PIPELINE_HANDLER` macro uses the name to construct C++ identifiers.

**Correct Names:**
- ✅ `softisp`
- ✅ `dummysoftisp`
- ❌ `virtual-softisp`
- ❌ `dummy-softisp`

**Why:** C++ identifiers cannot contain hyphens. The macro expands to something like `global_PipelineHandlerNameFactory`, which would be invalid with hyphens.

---

## 9. Forward Declarations in Headers

**Finding:** When a class (e.g., `SoftISPCameraData`) uses a pointer to another class (e.g., `PipelineHandlerSoftISP`) in its constructor, the latter must be **forward-declared** before the former.

**Correct Pattern:**
```cpp
namespace libcamera {
    class PipelineHandlerSoftISP; // Forward declaration
    
    class SoftISPCameraData : public Camera::Private {
    public:
        SoftISPCameraData(PipelineHandlerSoftISP *pipe); // OK
    };
    
    class PipelineHandlerSoftISP : public PipelineHandler {
        // ...
    };
}
```

---

## 10. Build Configuration on Termux

**Finding:** The `virtual` pipeline has a pre-existing compatibility issue with `strverscmp` on Termux. It must be fixed or excluded.

**Fix:**
```cpp
// In src/libcamera/pipeline/virtual/config_parser.cpp:
std::sort(files.begin(), files.end(), [](const auto &a, const auto &b) { 
    return a < b; // Fixed for Termux
});
```

**Build Command:**
```bash
meson setup build \
  -Dsoftisp=enabled \
  -Dpipelines='softisp,dummysoftisp' \
  -Dtest=true \
  -Dc_args=-Wno-error \
  -Dcpp_args=-Wno-error
```

---

## 11. Include Path for Local Headers

**Finding:** When creating local headers (e.g., `algorithm.h`, `module.h`) in an IPA module directory, add the current directory to the include path in `meson.build`.

**Correct Pattern:**
```python
softisp_includes = include_directories('.')

ipa_softisp = shared_module(ipa_name,
    softisp_module_sources,
    include_directories: [ipa_includes, softisp_includes],
    # ...
)
```

---

## 12. Reusing Existing Module Definitions

**Finding:** For IPA modules that don't need custom context types, reuse the `Module` definition from the `simple` pipeline.

**Correct Pattern:**
```cpp
// In src/ipa/softisp/module.h:
#include "../simple/module.h"
```

**Why:** This avoids duplicating complex template definitions and ensures compatibility with the `ipa::soft` namespace.

---

## Summary of File Structure

```
src/ipa/softisp/
├── algorithm.h          # Defines Algorithm type alias
├── module.h             # Defines Module type (reuses simple/module.h)
├── softisp.h            # SoftIsp class (inherits IPASoftInterface + Module)
├── softisp.cpp          # SoftIsp implementation
├── softisp_module.cpp   # ipaCreate() for "softisp" pipeline
└── softisp_virtual_module.cpp  # ipaCreate() for "dummysoftisp" pipeline

src/libcamera/pipeline/softisp/
├── softisp.h            # Pipeline handler and CameraData classes
└── softisp.cpp          # Pipeline implementation

src/libcamera/pipeline/dummysoftisp/
├── softisp.h            # Pipeline handler and CameraData classes
└── softisp.cpp          # Pipeline implementation
```

---

## Next Steps

1. **Implement ONNX Inference:** Fill in the `processStats()` method in `softisp.cpp` to run the ONNX models.
2. **Test App:** Fix the `softisp-test-app.cpp` to use the correct API (avoid deprecated methods).
3. **Unit Tests:** Implement the IPA module tests.
4. **Integration Tests:** Test with real and dummy cameras.

---

*Last Updated: 2026-04-21*
*Author: SoftISP Development Team*

---

## 13. Implementation Status: What's Complete vs. Stubbed

### ✅ **Fully Implemented:**
- **IPA Module Registration**: Both `ipa_softisp.so` and `ipa_softisp_virtual.so` correctly register with `ipaCreate()`
- **Pipeline Handler Registration**: Both pipelines use `REGISTER_PIPELINE_HANDLER` correctly
- **Camera Data Initialization**: `init()` and `loadIPA()` methods work correctly
- **Request Completion**: `completeRequest()` is called properly in `queueRequestDevice()`
- **Thread Handling**: Proper use of `exit(0)` and `wait()` for thread termination
- **Log Categories**: Correctly defined and used in both pipelines

### ⚠️ **Stubbed / Placeholder Implementations:**

#### **Both Pipelines (softisp & dummysoftisp):**

1. **`exportFrameBuffers()`** - Returns 0 without allocating actual buffers
   - **Current**: Creates empty `FrameBuffer` objects
   - **Needed**: Allocate DMA buffers using `dma_buf_allocator` or similar
   - **Impact**: Cannot capture real frames without this

2. **`start()`** - Just logs and returns 0
   - **Current**: No actual hardware initialization
   - **Needed**: Configure V4L2 device, set formats, start streaming
   - **Impact**: Camera won't actually start streaming

3. **`stopDevice()`** - Just logs and returns
   - **Current**: No cleanup of hardware resources
   - **Needed**: Stop streaming, release buffers, close device
   - **Impact**: Resource leaks on stop

4. **`processRequest()`** (in CameraData) - Completes request without processing
   - **Current**: Calls `completeRequest()` immediately
   - **Needed**: 
     - Extract statistics from captured frame
     - Pass to IPA module's `processStats()`
     - Apply metadata to frame
     - Then complete request
   - **Impact**: No actual ISP processing happens

#### **IPA Module (softisp.cpp):**

5. **`processStats()`** - Not implemented
   - **Current**: Returns 0 without doing anything
   - **Needed**: 
     - Load ONNX models (`algo.onnx` and `applier.onnx`)
     - Run inference to generate ISP coefficients
     - Apply coefficients to frame
     - Return metadata
   - **Impact**: Core ISP functionality missing

6. **`computeParams()`** - Not implemented
   - **Current**: Returns 0
   - **Needed**: Pre-compute parameters if needed
   - **Impact**: May affect performance

#### **Dummy Pipeline Only:**

7. **`generateConfiguration()`** - Now implemented with fixed resolution
   - **Current**: Returns config with 1920x1080 UYVY8
   - **Status**: ✅ Fixed (was returning nullptr)

#### **Real Pipeline Only:**

8. **`generateConfiguration()`** - Now implemented with fixed resolution
   - **Current**: Returns config with 1920x1080 BGGR8
   - **Status**: ✅ Fixed (was returning nullptr)

---

## 14. Next Steps for Full Implementation

### **Priority 1: ONNX Integration**
1. Implement `processStats()` in `src/ipa/softisp/softisp.cpp`
2. Add ONNX Runtime dependency to `meson.build`
3. Load `algo.onnx` and `applier.onnx` models
4. Run inference pipeline

### **Priority 2: Buffer Allocation**
1. Implement `exportFrameBuffers()` with actual DMA buffer allocation
2. Use `libcamera::DMABufAllocator` or V4L2 buffer allocation
3. Ensure proper buffer cleanup in destructor

### **Priority 3: V4L2 Integration (Real Pipeline)**
1. Open V4L2 device in `configure()`
2. Set video format and parameters
3. Implement `start()` to begin streaming
4. Implement `stopDevice()` to stop streaming
5. Implement `queueRequestDevice()` to queue buffers

### **Priority 4: Test App**
1. Fix API usage in `tools/softisp-test-app.cpp`
2. Implement proper camera enumeration
3. Add frame capture and saving functionality
4. Test with both pipelines

### **Priority 5: Unit Tests**
1. Complete `test/ipa/softisp_module_test.cpp`
2. Complete `test/ipa/softisp_virtual_module_test.cpp`
3. Add integration tests

---

## 15. Known Limitations (Current State)

1. **No Actual Frame Capture**: Buffers are not allocated, so no real frames can be captured
2. **No ISP Processing**: ONNX models are not loaded or executed
3. **No Hardware Integration**: V4L2 device is not opened or configured
4. **No Streaming**: Camera cannot start/stop streaming
5. **Test App Disabled**: Due to API changes, test app is not building

---

## 16. Build Verification Commands

```bash
# Full build (excluding tests)
meson compile -C build

# Build only library and IPA modules
ninja -C build src/libcamera/libcamera.so src/ipa/softisp/ipa_softisp.so src/ipa/softisp/ipa_softisp_virtual.so

# Verify built files
ls -lh build/src/libcamera/libcamera.so
ls -lh build/src/ipa/softisp/*.so

# Check IPA module symbols
nm -D build/src/ipa/softisp/ipa_softisp.so | grep ipaCreate
nm -D build/src/ipa/softisp/ipa_softisp_virtual.so | grep ipaCreate
```

---

*Last Updated: 2026-04-21*
*Status: Build Complete, Core Infrastructure Ready, ONNX Integration Pending*
