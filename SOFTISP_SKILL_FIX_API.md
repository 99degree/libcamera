# SoftISP API Fix Skill

## Objective
Fix remaining ControlList API usage and processFrame pointer passing in the SoftISP pipeline to achieve full compilation.

## Context
The SoftISP pipeline has two main compilation errors remaining:
1. `ControlList::merge()` called on a const object
2. `processFrame()` signature mismatch (passing reference vs pointer)

## Steps

### 1. Fix ControlList merge() Usage
The `metadata()` method returns a `const ControlList&`, but `merge()` requires a non-const reference.
**Solution**: Use `controls()` instead, which returns `ControlList&`.

**File**: `src/libcamera/pipeline/softisp/softisp.cpp`
**Change**:
```cpp
// Before (error):
request->metadata().merge(statsResults, ControlList::MergePolicy::OverwriteExisting);

// After (correct):
request->controls().merge(statsResults, libcamera::ControlList::MergePolicy::OverwriteExisting);
```

### 2. Fix processFrame() Pointer Passing
The `processFrame()` method expects a `ControlList*` pointer, not a reference.
**Solution**: Pass the pointer directly instead of dereferencing it.

**File**: `src/libcamera/pipeline/softisp/softisp.cpp`
**Change**:
```cpp
// Before (error):
ControlList *resultsPtr = &request->controls();
int32_t ret = ipa_->processFrame(frameId, bufferId, plane.fd, 0,
                                 streamConfig.size.width, streamConfig.size.height,
                                 *resultsPtr); // Passing reference

// After (correct):
ControlList *resultsPtr = &request->controls();
int32_t ret = ipa_->processFrame(frameId, bufferId, plane.fd, 0,
                                 streamConfig.size.width, streamConfig.size.height,
                                 resultsPtr); // Passing pointer
```

### 3. Fix IPA Proxy Type
The `IPAManager::createIPA()` returns `IPAProxySoftIspThreaded` by default.
**Solution**: Use the correct template parameter.

**File**: `src/libcamera/pipeline/softisp/softisp.cpp`
**Change**:
```cpp
// Before:
ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIspIsolated>(...);

// After:
ipa_ = IPAManager::createIPA<ipa::soft::IPAProxySoftIspThreaded>(...);
```

## Verification
After applying these fixes:
```bash
cd /data/data/com.termux/files/home/libcamera2
meson compile -C build
```

Expected result: No errors in `pipeline_softisp_softisp.cpp.o`

## Files Modified
- `src/libcamera/pipeline/softisp/softisp.cpp`
- `src/libcamera/pipeline/softisp/softisp.h` (if needed)

## Next Steps After Compilation
1. Implement actual ONNX inference logic in `src/ipa/softisp/softisp.cpp`
2. Add real camera V4L2 support if needed
3. Test with ONNX models (algo.onnx, applier.onnx)
