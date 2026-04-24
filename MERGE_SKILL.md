# Skill: Splitting and Merging C++ Classes into Separate Files

> **Author:** George Chan <gchan9527@gmail.com>  
> **Context:** Refactoring `SoftISP` pipeline to align with `SimplePipeline` pattern  
> **Goal:** Split `SoftISPCameraData` into its own file (`softisp_camera.cpp/.h`) while keeping `PipelineHandlerSoftISP` in `softisp.cpp`

---

## 🎯 Problem Statement

The original `softisp.cpp` contained both:
1. `PipelineHandlerSoftISP` (Dispatcher)
2. `SoftISPCameraData` (Camera Object)

This made the file hard to maintain and trace. The goal was to:
- Move `SoftISPCameraData` to `softisp_camera.cpp` / `softisp_camera.h`
- Keep `PipelineHandlerSoftISP` in `softisp.cpp`
- Ensure both files compile cleanly

---

## 🛠️ Step-by-Step Merge/Split Guide

### 1. **Backup the Original File**
```bash
cp softisp.cpp softisp.cpp.backup
```
**Why:** The original file may be minified or have unique formatting. Always keep a reference.

### 2. **Create the New Header File (`softisp_camera.h`)**
Define the class declaration with proper includes and forward declarations.

```cpp
/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include <memory>
#include <vector>

#include <libcamera/base/mutex.h>
#include <libcamera/base/thread.h>
#include "libcamera/internal/camera.h"

namespace libcamera {

class PipelineHandlerSoftISP;
class VirtualCamera;
class SoftISPConfiguration;

class SoftISPCameraData : public Camera::Private, public Thread {
public:
    SoftISPCameraData(PipelineHandlerSoftISP *pipe);
    ~SoftISPCameraData();

    int init();
    std::unique_ptr<CameraConfiguration> generateConfiguration(Span<const StreamRole> roles);
    // ... other methods

    VirtualCamera *virtualCamera() { return virtualCamera_.get(); }

private:
    void run() override;
    std::unique_ptr<VirtualCamera> virtualCamera_;
    Mutex mutex_;
};

} // namespace libcamera
```

**Key Points:**
- Use `#pragma once`
- Include only necessary headers
- Use forward declarations for classes not fully needed in the header
- Declare all methods that will be implemented in the `.cpp` file

### 3. **Create the New Implementation File (`softisp_camera.cpp`)**
Start with the header, includes, and log category **inside the namespace**.

```cpp
/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp_camera.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <libcamera/base/log.h>
#include <libcamera/camera.h>
#include <libcamera/controls.h>
#include <libcamera/formats.h>
#include <libcamera/stream.h>

#include "softisp.h"
#include "virtual_camera.h"

namespace libcamera {

// ⚠️ IMPORTANT: LOG_DECLARE_CATEGORY must be INSIDE the namespace
LOG_DECLARE_CATEGORY(SoftISPCameraData)

SoftISPCameraData::SoftISPCameraData(PipelineHandlerSoftISP *pipe)
    : Camera::Private(pipe),
      Thread("SoftISPCamera"),
      virtualCamera_(std::make_unique<VirtualCamera>())
{
    LOG(SoftISPCameraData, Info) << "SoftISPCameraData created";
}

// ... other methods

} // namespace libcamera
```

**Common Pitfall:**
- ❌ `LOG_DECLARE_CATEGORY` **before** `namespace libcamera {` → `unknown type name 'LogCategory'`
- ✅ `LOG_DECLARE_CATEGORY` **inside** `namespace libcamera {` → Compiles successfully

### 4. **Update the Main Handler File (`softisp.cpp`)**
Remove the `SoftISPCameraData` class definition and methods. Keep only `PipelineHandlerSoftISP`.

```cpp
/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"

// ... includes

#include "softisp_camera.h"  // ⚠️ Include the new header
#include "virtual_camera.h"

namespace libcamera {

// ⚠️ IMPORTANT: LOG_DEFINE_CATEGORY must be INSIDE the namespace
LOG_DEFINE_CATEGORY(SoftISPPipeline)
LOG_DEFINE_CATEGORY(SoftISPCameraData)  // Define the category used by the other file

// Forward declaration (if needed before the include)
class SoftISPCameraData;

// PipelineHandlerSoftISP implementation...
bool PipelineHandlerSoftISP::match(...) {
    // Use SoftISPCameraData
    std::unique_ptr<SoftISPCameraData> data = std::make_unique<SoftISPCameraData>(this);
    // ...
}

} // namespace libcamera
```

**Key Points:**
- Include `softisp_camera.h` to get the full class definition
- Use `LOG_DEFINE_CATEGORY` for the main file (defines the actual log category instance)
- Use `LOG_DECLARE_CATEGORY` in the split file (declares the existing instance)

### 5. **Update `meson.build`**
Add the new source file to the build.

```meson
libcamera_internal_sources += files([
    'softisp.cpp',
    'softisp_camera.cpp',  # ⚠️ Add this
    'virtual_camera.cpp',
])
```

### 6. **Update the Original Header (`softisp.h`)**
Ensure it includes the new header and has forward declarations.

```cpp
#pragma once

// ... includes

#include "softisp_camera.h"  // ⚠️ Include the new header
#include "virtual_camera.h"

namespace libcamera {

// Forward declaration if needed
class SoftISPCameraData;

class PipelineHandlerSoftISP : public PipelineHandler {
    // ...
    SoftISPCameraData *cameraData(Camera *camera) const;
};

} // namespace libcamera
```

---

## 🐛 Common Errors & Fixes

### Error 1: `unknown type name 'LogCategory'`
**Cause:** `LOG_DECLARE_CATEGORY` or `LOG_DEFINE_CATEGORY` is placed **outside** the `namespace libcamera {` block.

**Fix:** Move the macro **inside** the namespace.
```cpp
namespace libcamera {
    LOG_DECLARE_CATEGORY(SoftISPCameraData)  // ✅ Correct
}
```

### Error 2: `incomplete type 'libcamera::SoftISPCameraData'`
**Cause:** `softisp.cpp` tries to use `SoftISPCameraData` methods but only has a forward declaration.

**Fix:** Include `softisp_camera.h` in `softisp.cpp`.
```cpp
#include "softisp_camera.h"  // ✅ Full definition available
```

### Error 3: `out-of-line definition does not match any declaration`
**Cause:** Method signature in `.cpp` doesn't match the header.

**Fix:** Ensure exact match (including `const`, `static`, return types).
```cpp
// Header
std::unique_ptr<CameraConfiguration> generateConfiguration(Span<const StreamRole> roles);

// Implementation
std::unique_ptr<CameraConfiguration> SoftISPCameraData::generateConfiguration(
    Span<const StreamRole> roles)  // ✅ Must match exactly
```

### Error 4: Minified/Single-Line Code
**Cause:** Original file has no newlines (common in generated or minified code).

**Fix:** Manually rewrite the file with proper formatting. Do not try to auto-split minified code.
```python
# Manual rewrite is safer than regex splitting
with open('softisp_camera.cpp', 'w') as f:
    f.write(manually_formatted_code)
```

---

## ✅ Verification Checklist

- [ ] `softisp_camera.h` has `#pragma once` and forward declarations
- [ ] `softisp_camera.cpp` includes `softisp_camera.h` and necessary system headers
- [ ] `LOG_DECLARE_CATEGORY` is **inside** `namespace libcamera` in `softisp_camera.cpp`
- [ ] `LOG_DEFINE_CATEGORY` is **inside** `namespace libcamera` in `softisp.cpp`
- [ ] `softisp.cpp` includes `softisp_camera.h`
- [ ] `meson.build` includes `softisp_camera.cpp`
- [ ] No tabs in C/C++ files (use 4 spaces)
- [ ] All methods declared in header are implemented in `.cpp`
- [ ] Build succeeds with `meson compile -C build`

---

## 🚀 Result

After splitting:
- **`softisp.cpp`**: Only `PipelineHandlerSoftISP` (Dispatcher)
- **`softisp_camera.cpp`**: Only `SoftISPCameraData` (Camera Object)
- **`softisp_camera.h`**: Declaration of `SoftISPCameraData`
- **Clean separation**: Easy to trace, maintain, and extend

---

**Author:** George Chan <gchan9527@gmail.com>  
**Last Updated:** 2026-04-24
