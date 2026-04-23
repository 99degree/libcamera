# MOJOM Interface Integration - Progress Report

## ✅ What's Working

### 1. MOJOM Generation
- MOJOM files are now **automatically generated** when building with `-Dpipelines=softisp`
- Generated files include:
  - `softisp_ipa_interface.h` - Interface definition
  - `softisp_ipa_proxy.h` - Proxy class (`IPAProxySoftIsp`)
  - `softisp_ipa_serializer.h` - Serialization support
  - `softisp_proxy_worker` - Proxy worker process

### 2. Build System
- Development mode enabled (`-Ddevelopment`)
- Signature verification bypassed
- ONNX Runtime integrated (1.25.0)
- Module builds successfully

### 3. MOJOM Definition
The `include/libcamera/ipa/softisp.mojom` file defines:
```mojom
interface IPASoftIspInterface {
  init(...) => (int32 ret, ControlList ipaControls, bool ccmEnabled);
  start() => (int32 ret);
  stop();
  configure(IPAConfigInfo) => (int32 ret);
  queueRequest(uint32 frame, ControlList sensorControls);
  computeParams(uint32 frame);
  processStats(uint32 frame, uint32 bufferId, ControlList sensorControls);
  processFrame(uint32 frame, uint32 bufferId, SharedFD bufferFd,
               int32 planeIndex, int32 width, int32 height,
               ControlList results);
};
```

## ⚠️ What Needs to be Done

### Interface Alignment
The `SoftIsp` class needs to be updated to implement `IPASoftIspInterface` instead of `IPASoftInterface`:

**Changes needed:**
1. Change base class: `IPASoftInterface` → `IPASoftIspInterface`
2. Update include: `soft_ipa_interface.h` → `softisp_ipa_interface.h`
3. Fix method signatures to match MOJOM exactly:
   - `processStats`: 3rd param is `const ControlList &sensorControls` (not `ControlList &stats`)
   - `processFrame`: 
     - 1st param: `const uint32_t frame` (not `frameId`)
     - 4th param: `const int32_t planeIndex` (not `offset`)
     - 5th-6th params: `const int32_t` (not `uint32_t`)
     - 7th param: `const ControlList &results` (not `ControlList *results`)
4. Add `override` to `processFrame`
5. Remove `override` from `logPrefix()` (not virtual in base)
6. Fix `IPACameraSensorInfo` usage:
   - Use `outputSize.width/height` instead of `activeArea.width/height`

## 📊 Build Status

```bash
$ meson setup softisp_only -Dpipelines=softisp -Dsoftisp=enabled -Ddevelopment=true
$ ninja -C softisp_only
[1/167] Generating softisp_mojom_module
[31/167] Generating softisp_proxy_h  ← MOJOM proxy generated!
[167/167] Linking target src/v4l2/v4l2-compat.so
```

## 🎯 Next Steps

1. **Update SoftIsp class** to implement `IPASoftIspInterface`
2. **Fix all method signatures** to match MOJOM exactly
3. **Rebuild** and verify module loads
4. **Test** with `cam --list`

## 📁 Key Files

```
include/libcamera/ipa/softisp.mojom          - MOJOM definition
softisp_only/include/libcamera/ipa/softisp_ipa_proxy.h - Generated proxy
src/ipa/softisp/softisp.h                    - Needs update
src/ipa/softisp/softisp.cpp                  - Needs update
```

---

**Branch**: `feature/softisp-ipa-onnx`  
**Status**: MOJOM generation working, interface alignment in progress  
**Commit**: `2cc2c47`
