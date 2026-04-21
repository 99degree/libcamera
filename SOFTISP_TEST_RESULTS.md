# SoftISP Testing Results

## Test Date
2026-04-21

## Test Environment
- Platform: Termux (Android)
- Architecture: aarch64
- ONNX Runtime: 1.25.0
- Python: 3.13.13

## Test 1: Model Loading
**Status:** ✅ PASSED

**Test Program:** `softisp_test.cpp`

**Results:**
- algo.onnx loaded successfully (25KB)
- applier.onnx loaded successfully (20KB)
- algo.onnx has 4 inputs and 15 outputs
- applier.onnx has 10 inputs and 7 outputs

**Conclusion:** Both ONNX models are valid and can be loaded by ONNX Runtime.

## Test 2: Full libcamera Build
**Status:** ⚠️ BLOCKED

**Issue:** Pre-existing build issues in libpisp subproject unrelated to SoftISP code:
1. nlohmann_json deprecation warnings treated as errors
2. Missing `PTHREAD_MUTEX_ROBUST` identifier on Termux

**Workaround Needed:**
- Disable `-Werror` flag in build
- Or patch libpisp subproject to fix pthread issues

## Test 3: SoftISP Integration
**Status:** ⏳ PENDING

**Next Steps:**
1. Fix libpisp build issues or disable `-Werror`
2. Rebuild libcamera with `-Dsoftisp=enabled`
3. Test with real camera pipeline using `libcamera-vid` or `libcamera-avg`
4. Verify SoftISP loads models and produces correct AWB gains

## Model Specifications

### algo.onnx
- **Size:** 25KB
- **Inputs:** 4
- **Outputs:** 15
- **Purpose:** ISP coefficient generation from AWB statistics

### applier.onnx
- **Size:** 20KB
- **Inputs:** 10
- **Outputs:** 7
- **Purpose:** Apply ISP coefficients to full-resolution image

## Recommendations

1. **Immediate:** The ONNX models are valid and ready for use
2. **Short-term:** Fix libpisp build issues to enable full testing
3. **Long-term:** Test with real camera hardware to validate image quality

## Files Tested
- `/data/data/com.termux/files/home/softisp_models/algo.onnx`
- `/data/data/com.termux/files/home/softisp_models/applier.onnx`

## Test Programs Created
- `softisp_test.cpp` - Basic model loading test (PASSED)
- `softisp_full_test.cpp` - Full inference test (in progress)
