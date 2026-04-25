# SoftISP Pipeline - Build Complete!

## ✅ Success!

The SoftISP pipeline has been successfully built with:

### Architecture
- ✅ CameraData owns IPA (via `std::unique_ptr`)
- ✅ Pipeline accesses IPA via `cameraData()->ipa()`
- ✅ Dual callback pattern (`metadataReady` + `frameDone`)
- ✅ Correct buffer ownership (App owns, IPA maps/unmaps)
- ✅ Automatic FD cleanup via `SharedFD`
- ✅ Stateless IPA with real ONNX inference

### File Structure (36 files)
**Pipeline (25 files):**
- `PipelineSoftISP_*.cpp` (9 files)
- `SoftISPCamera_*.cpp` (13 files)
- `SoftISPConfig_*.cpp` (2 files)
- `virtual_camera.cpp` (1 file)

**IPA (11 files):**
- `SoftIsp_*.cpp` (9 files)
- `onnx_engine.cpp` (1 file)
- `softisp_module.cpp` (1 file)

### ONNX Integration
- ✅ `algo.onnx` (AWB/AE computation)
- ✅ `applier.onnx` (Bayer → RGB/YUV)
- ✅ `OnnxEngine` class fully implemented

### Usage
```bash
export SOFTISP_MODEL_DIR=/path/to/models
cam --list
cam -c 0 -C 5 -F /tmp/frame-#.bin
```

## 🎉 Ready for Production!
