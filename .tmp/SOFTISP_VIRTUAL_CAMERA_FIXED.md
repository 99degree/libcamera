# SoftISP Virtual Camera - Infinite Loop Fixed ✅

## Problem
The SoftISP pipeline was stuck in an infinite loop where:
1. `match()` was called repeatedly
2. A virtual camera was created each time
3. The pipeline handler was destroyed immediately after
4. The cycle repeated infinitely, preventing `cam --list` from completing

## Root Causes
1. **Incorrect return value**: `match()` returned `true` even when the virtual camera was already created, causing the CameraManager to destroy and recreate the pipeline.
2. **Missing camera registration**: The camera was created with `Camera::create()` but not properly registered with `registerCamera()`.
3. **Namespace issue**: The `REGISTER_PIPELINE_HANDLER` macro was causing compilation errors due to namespace mismatch.

## Solutions Applied

### 1. Fixed `match()` Logic (`PipelineSoftISP_match.cpp`)
```cpp
bool PipelineHandlerSoftISP::match(DeviceEnumerator *enumerator)
{
    (void)enumerator;
    LOG(SoftISPPipeline, Info) << "match() called";
    
    // If we've already created the virtual camera, don't match again
    if (created_) {
        LOG(SoftISPPipeline, Info) << "Virtual camera already registered, skipping";
        return false;  // ← Key fix: return false to stop the loop
    }
    
    created_ = true;
    // ... create camera ...
    
    registerCamera(std::move(camera));  // ← Key fix: properly register
    return true;
}
```

### 2. Manual Factory Registration (`softisp.cpp`)
Replaced the problematic macro with manual registration:
```cpp
namespace libcamera {
static PipelineHandlerFactory<PipelineHandlerSoftISP> global_PipelineHandlerSoftISPFactory("softisp");
} /* namespace libcamera */
```

## Result
✅ Virtual camera now appears in `cam --list`:
```
Available cameras:
1: (softisp_virtual)
```

✅ No more infinite loop - `match()` is called once, creates the camera, and returns `false` on subsequent calls.

✅ Camera persists and is properly registered with the CameraManager.

## Next Steps
1. Test frame capture with the virtual camera
2. Implement real ONNX model inference
3. Add statistics processing logic
