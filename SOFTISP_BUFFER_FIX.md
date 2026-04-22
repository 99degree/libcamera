# SoftISP Buffer Output Fix

## Problem
The current SoftISP implementation runs `algo.onnx` and `applier.onnx` successfully, but **discards the output**. The processed image is never written back to the `FrameBuffer`, so the test app receives the original Bayer pattern (or garbage) instead of the processed image.

## Root Cause
- **Input**: The pipeline correctly reads the Bayer pattern from the `FrameBuffer`.
- **Processing**: `applier.onnx` generates the processed image tensor.
- **Output**: The tensor data is **not copied** back to the `FrameBuffer` memory.
- **Result**: The `FrameBuffer` passed to the app remains unchanged.

## Solution: In-Place Buffer Writing
Standard libcamera pipelines use **in-place processing**:
1. The app allocates a buffer (e.g., 1920x1080x3 bytes).
2. The pipeline reads from it (Input).
3. The pipeline writes the result **back into the same memory** (Output).
4. The app reads the processed data from that same buffer.

**No extra buffer allocation is needed.** We simply need to map the existing buffer memory and copy the ONNX output tensor into it.

## Implementation Steps

### 1. Store Buffer Pointers in Pipeline
In `src/libcamera/pipeline/dummysoftisp/softisp.cpp`:
- Add a `std::map<uint32_t, FrameBuffer*> bufferMap` to `DummySoftISPCameraData`.
- In `queueRequestDevice()`, store the buffer pointer before calling `processStats()`.
- In `processRequest()`, retrieve the buffer pointer using `bufferId`.

### 2. Write Output to Buffer in IPA
In `src/ipa/softisp/softisp.cpp` (`SoftIsp::processStats()`):
- After `applier.onnx` completes, get the output tensor.
- Map the `FrameBuffer` memory using `mmap()`.
- Copy/convert the tensor data into the buffer.
- Unmap the memory.

### 3. Handle Format Mismatch
- **Current Stream Format**: `NV12` (YUV).
- **Model Output**: Likely `RGB` or `Grayscale` (float/uint8).
- **Fix**: Either:
  - **Option A (Recommended for testing)**: Change stream format to `RGB888` in `generateConfiguration()`.
  - **Option B (Production)**: Implement RGB -> NV12 color space conversion before writing.

## Code Changes Required

### `src/libcamera/pipeline/dummysoftisp/softisp.h`
```cpp
class DummySoftISPCameraData {
    // ... existing members ...
    std::map<uint32_t, FrameBuffer*> bufferMap; // Map bufferId -> FrameBuffer*
};
```

### `src/libcamera/pipeline/dummysoftisp/softisp.cpp`
```cpp
// In queueRequestDevice()
data->bufferMap[bufferId] = buffer; // Store before processing
data->processRequest(request);
// Optional: Clean up later
// data->bufferMap.erase(bufferId);

// In processRequest() or a helper
FrameBuffer* getBufferFromId(uint32_t bufferId) {
    auto it = cameraData->bufferMap.find(bufferId);
    return (it != cameraData->bufferMap.end()) ? it->second : nullptr;
}
```

### `src/ipa/softisp/softisp.cpp`
```cpp
// In SoftIsp::processStats() after applier.onnx
const auto& outputTensor = applierOutputs[0]; // Adjust index if needed
FrameBuffer* buffer = getBufferFromId(bufferId); // Implement lookup

if (!buffer || buffer->planes().empty()) return -EINVAL;

const auto& plane = buffer->planes()[0];
void* bufferMem = mmap(nullptr, plane.length, PROT_READ | PROT_WRITE, MAP_SHARED, plane.fd.get(), 0);
if (bufferMem == MAP_FAILED) return -errno;

const float* tensorData = outputTensor.GetTensorData<float>();
size_t tensorElements = outputTensor.GetTensorTypeAndShapeInfo().GetElementCount();

// Convert float [0.0-1.0] to uint8 [0-255]
uint8_t* dest = static_cast<uint8_t*>(bufferMem);
for (size_t i = 0; i < tensorElements; ++i) {
    float val = std::clamp(tensorData[i], 0.0f, 1.0f);
    dest[i] = static_cast<uint8_t>(val * 255.0f);
}

munmap(bufferMem, plane.length);
LOG(SoftIsp, Info) << "Successfully wrote processed image to buffer";
```

## Verification
1. Change stream format to `RGB888` in `generateConfiguration()` (temporarily).
2. Run `softisp-save` or `libcamera-cam`.
3. Check logs for "Successfully wrote processed image to buffer".
4. Verify saved files contain processed image data (not Bayer pattern).
