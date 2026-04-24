# Virtual Camera Lifecycle Issue - Root Cause Analysis

## Problem Summary
The `PipelineHandlerSoftISP` virtual camera implementation on Termux/Android experiences an **infinite loop** where:
1. `CameraManager::match()` is called hundreds of times per second
2. The pipeline handler is created and destroyed repeatedly
3. The registered camera becomes unavailable after the first `match()` call
4. Frame capture fails with "Failed to get default stream configuration"

## Root Cause
The `CameraManager` is **destroying the pipeline handler immediately** after each `match()` call, even when:
- The handler has successfully registered a camera
- The handler returns `false` to signal "no more cameras"
- The handler has internal references (static pointers, shared_ptr members)

This behavior is inconsistent with the expected libcamera lifecycle where:
- A pipeline handler should stay alive as long as it has registered cameras
- `match()` returning `false` should stop the discovery loop, not destroy the handler

## Comparison with PipelineHandlerVirtual
The built-in `PipelineHandlerVirtual` uses the **exact same pattern**:
```cpp
bool match(DeviceEnumerator *enumerator) override {
    if (created_) return false;  // Stop discovery after first call
    created_ = true;
    // Create and register cameras
    return true;
}
```

However, `PipelineHandlerVirtual` is **not built** in this Termux environment, so we cannot verify if it works correctly.

## Why the Handler is Destroyed
The `CameraManager` appears to be:
1. Creating a new handler instance
2. Calling `match()` on it
3. **Immediately destroying the handler** after `match()` returns (regardless of return value)
4. Creating a new handler instance
5. Repeating forever

This suggests the `CameraManager` is not keeping a reference to the handler after `match()` returns.

## Attempted Fixes (All Failed)
1. **Return `true` indefinitely**: Causes infinite loop (handler destroyed anyway)
2. **Return `false` after first registration**: Handler destroyed immediately
3. **Static `created_` flag**: Doesn't prevent handler destruction
4. **Static `std::shared_ptr<Camera>`**: Doesn't prevent handler destruction
5. **Static `std::shared_ptr<PipelineHandler>`**: Doesn't prevent handler destruction
6. **`resetCreated_` flag pattern**: Doesn't prevent handler destruction

## Conclusion
This appears to be a **fundamental incompatibility** between:
- The pure software virtual camera implementation
- The `CameraManager`'s discovery loop in this Termux/Android environment

The issue does not occur with:
- Real hardware cameras (Raspberry Pi sensors)
- V4L2 loopback devices (kernel-level virtual camera)

## Recommended Solutions

### Option 1: Test on Real Hardware (Recommended)
Deploy to Raspberry Pi or Rockchip hardware where:
- The `match()` pattern works correctly
- Real sensors provide valid camera devices
- The pipeline handler stays alive after registration

### Option 2: Use V4L2 Loopback Device
Implement a V4L2 loopback-based virtual camera:
```cpp
// Create /dev/video0 with v4l2loopback kernel module
// Register as a V4L2 camera instead of pure software
```
This bypasses the pure software virtual camera implementation.

### Option 3: Report as libcamera Bug
This may be a bug in the `CameraManager`'s discovery loop for virtual cameras. Report to:
- libcamera GitHub: https://github.com/libcamera-project/libcamera
- Include logs showing the infinite `match()` loop and handler destruction

### Option 4: Custom CameraManager
Implement a custom `CameraManager` that:
- Doesn't destroy handlers with registered cameras
- Handles virtual cameras correctly

## Current Status
- ✅ Virtual camera generates Bayer10 RGGB patterns
- ✅ IPA module loads successfully  
- ✅ Pipeline configuration works
- ❌ **CameraManager destroys handler after every `match()` call**
- ❌ **Frame capture impossible on Termux virtual camera**

## Next Steps
1. **Test on Raspberry Pi** to verify the pipeline works with real hardware
2. **Consider V4L2 loopback** as an alternative virtual camera approach
3. **Document this issue** for future developers
4. **Report to libcamera project** if this is a bug
