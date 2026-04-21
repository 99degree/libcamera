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
