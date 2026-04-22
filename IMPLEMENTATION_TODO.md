# SoftISP Full Implementation - TODO

## Current Status
- ✅ Mojom interface updated with `processFrame` method
- ✅ `processFrame` method added to IPA (`softisp.cpp`)
- ✅ `processFrame` declaration added to IPA header
- ❌ Pipeline handlers need to call `processFrame`
- ❌ Build system needs to regenerate mojom files

## What Was Done

### 1. Updated Mojom Interface
File: `include/libcamera/ipa/softisp.mojom`
- Added `processFrame` method with buffer FD, stride, dimensions, and results

### 2. Implemented processFrame in IPA
File: `src/ipa/softisp/softisp.cpp`
- Maps buffer from FD
- Applies ONNX output data to buffer
- Writes metadata (AWB gains, focus score) to results
- Unmaps buffer

### 3. Added Declaration
File: `src/ipa/softisp/softisp.h`
- Added `processFrame` override declaration

## Next Steps

### Step 1: Rebuild to Generate Mojom Files
```bash
cd build
meson compile
```
This will generate the proxy/stub files from the mojom interface.

### Step 2: Update Pipeline Handler (dummysoftisp)
File: `src/libcamera/pipeline/dummysoftisp/softisp.cpp`

In `DummySoftISPCameraData::processRequest()`:
1. After calling `ipa_->processStats()`, call:
```cpp
ipa_->processFrame(frameId, bufferId, plane.fd, 0, plane.stride,
                   streamConfig.size.width, streamConfig.size.height,
                   &request->metadata());
```
2. Complete the request with:
```cpp
pipe()->completeRequest(request);
```

### Step 3: Update Real Pipeline (softisp)
File: `src/libcamera/pipeline/softisp/softisp.cpp`
- Similar changes as dummysoftisp
- Add V4L2 device handling

### Step 4: Test
```bash
./build/tools/softisp-test-app --pipeline dummysoftisp --frames 10
```

## Files Modified
- `include/libcamera/ipa/softisp.mojom`
- `src/ipa/softisp/softisp.cpp`
- `src/ipa/softisp/softisp.h`

## Files to Modify
- `src/libcamera/pipeline/dummysoftisp/softisp.cpp`
- `src/libcamera/pipeline/softisp/softisp.cpp`

## Notes
- The `processFrame` implementation currently writes float data as uint8_t
- For production, need to handle different pixel formats properly
- Focus score calculation is stubbed (needs real buffer access)
