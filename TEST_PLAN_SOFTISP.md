# SoftISP "No Camera" Integration Test Plan

**Date:** 2026-04-21  
**Status:** Ready for Execution  
**Target:** `dummy_softisp` pipeline (No hardware required)

---

## 1. Objective
Verify that the **SoftISP** Image Processing Algorithm (IPA) functions correctly within the **libcamera** framework using the `dummy_softisp` pipeline. This test validates the entire stack (Pipeline → IPA Module → ONNX Models) **without requiring any physical camera hardware**.

---

## 2. Prerequisites
Before starting, ensure the following are available:
1.  **Source Code**: The `libcamera` repository with the SoftISP implementation (25 commits).
2.  **ONNX Models**: `algo.onnx` and `applier.onnx` located in `$HOME/softisp_models/`.
3.  **Build Tools**: `meson`, `ninja`, `libonnxruntime-dev`, `g++`, `pkg-config`.

---

## 3. Build Instructions

### 3.1. Clean Previous Builds
```bash
cd /data/data/com.termux/files/home/libcamera
rm -rf build
```

### 3.2. Configure Meson
We enable the SoftISP feature, explicitly request both pipelines (`softisp` and `dummy_softisp`), and disable strict error checking to bypass known Termux-specific build warnings.

```bash
meson setup build \
  -Dsoftisp=enabled \
  '-Dpipelines=simple,softisp,dummy_softisp' \
  -Dtest=true \
  -Dc_args=-Wno-error \
  -Dcpp_args=-Wno-error
```

### 3.3. Compile
```bash
meson compile -C build
```

### 3.4. Verify Build Artifacts
Ensure the following files were created:
```bash
ls -lh \
  build/src/ipa/softisp/ipa_softisp.so \
  build/src/ipa/softisp/ipa_softisp_virtual.so \
  build/tools/softisp-test-app
```
*Expected: All files should exist and have non-zero size.*

---

## 4. Test Execution

### 4.1. Set Environment Variables
Point the application to the ONNX models and ensure shared libraries are found.

```bash
export SOFTISP_MODEL_DIR=$HOME/softisp_models
export LD_LIBRARY_PATH=$HOME/libcamera/build/src:$HOME/libcamera/build/src/libcamera:$LD_LIBRARY_PATH
```

### 4.2. Step 1: Unit Tests (IPA Module Loading)
Verify that the IPA modules load correctly and have the correct `pipelineName` configuration.

```bash
cd /data/data/com.termux/files/home/libcamera/build
meson test softisp_module_test softisp_virtual_module_test --verbose
```

**✅ Expected Result:**
- `softisp_module_test`: Passes. Output: `name = softisp, pipelineName = softisp`
- `softisp_virtual_module_test`: Passes. Output: `name = softisp-virtual, pipelineName = dummy_softisp`

### 4.3. Step 2: Integration Test (Virtual Pipeline)
Run the test application with the `dummy_softisp` pipeline to simulate a camera session.

```bash
./tools/softisp-test-app --pipeline dummy_softisp --output test_virtual.yuv --frames 10
```

**✅ Expected Output:**
```text
Available cameras:
  /dev/media0 (Pipeline: dummy_softisp)
Using camera: /dev/media0 (Pipeline: dummy_softisp)
Camera started. Processing 10 frames...
Processed frame 1/10
Processed frame 2/10
...
Processed frame 10/10
Saving output to test_virtual.yuv
Test completed successfully. Processed 10 frames.
```

### 4.4. Step 3: Output Verification
Check that the output file was generated and contains data.

```bash
ls -lh test_virtual.yuv
# Expected size: ~6MB (640x480x2 bytes * 10 frames, approx)
```

---

## 5. Troubleshooting

| Issue | Possible Cause | Solution |
| :--- | :--- | :--- |
| `No camera found for pipeline` | Pipeline not built or not in path | Ensure `-Dpipelines=...,dummy_softisp` was used. Check `build/src/libcamera/pipeline/dummy_softisp/`. |
| `Failed to create SoftISP IPA` | Model files missing | Verify `SOFTISP_MODEL_DIR` points to a folder containing `algo.onnx` and `applier.onnx`. |
| `ONNX Runtime Error` | Corrupted models or version mismatch | Re-download models from `https://github.com/99degree/softisp-python`. |
| `pthread_setaffinity_np` error | Build warning treated as error | Ensure `-Dc_args=-Wno-error` was used in `meson setup`. |

---

## 6. Success Criteria
The test is considered **PASSED** if:
1.  The build completes without fatal errors.
2.  Both IPA module unit tests pass.
3.  `softisp-test-app` runs successfully with `--pipeline dummy_softisp` **without a physical camera**.
4.  The app logs "Processed frame X" for all requested frames.
5.  An output file (`test_virtual.yuv`) is generated with non-zero size.

---

## 7. Conclusion
If all steps pass, the SoftISP implementation is **functionally complete** for the algorithmic layer. The system can now process images using ONNX models entirely in userspace, independent of hardware drivers.
