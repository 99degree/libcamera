# Feature: Virtual Camera Decoupling

**Branch:** `feature/softisp-virtual-decoupled`  
**Base:** `feature/softisp-full-inference`  
**Status:** 🟢 Ready to Implement  
**Created:** 2026-04-23

---

## What's Been Done ✅

### 1. Analysis Complete
- ✅ **Virtual Pipeline Transplant Report**: Full git history analysis showing `dummysoftisp` was transplanted from `virtual` pipeline
- ✅ **Decoupling Guide**: Comprehensive documentation on creating reusable virtual camera classes
- ✅ **Refactoring Plan**: Step-by-step guide for refactoring `dummysoftisp` to use the stub

### 2. Stub Class Created
- ✅ **File**: `src/libcamera/internal/virtual_camera_stub.h`
- ✅ **Size**: 150 lines (header-only, zero dependencies)
- ✅ **Features**:
  - Thread-based request processing
  - Automatic request queuing with mutex
  - Frame generation hook (override for custom logic)
  - Simple start/stop API
  - Sequence number tracking

### 3. Documentation Created
- ✅ `VIRTUAL_PIPELINE_TRANSPLANT_REPORT.md` - Full history analysis
- ✅ `docs/DECOUPLING_VIRTUAL_CAMERA.md` - Design patterns and options
- ✅ `docs/REFACTOR_DUMMYSOFTISP_WITH_STUB.md` - Before/after code comparison
- ✅ `COMPREHENSIVE_ANALYSIS.md` - Overall project status

---

## What's Next 🚀

### Immediate Tasks (Priority 1)

#### 1. Update `dummysoftisp` to Use Stub
**Files to Modify:**
- `src/libcamera/pipeline/dummysoftisp/softisp.h`
- `src/libcamera/pipeline/dummysoftisp/softisp.cpp`

**Changes:**
```cpp
// Before
class DummySoftISPCameraData : public Camera::Private, public Thread {
    void run() override;  // ~30 lines
    void processRequest(Request *request);  // ~50 lines
};

// After
class DummySoftISPCameraData : public Camera::Private, public VirtualCameraStub {
protected:
    void processRequest(Request *request) override;  // ~20 lines (IPA logic only)
    int generateFrame(FrameBuffer *buffer, uint32_t seq) override;  // Optional
};
```

**Expected Result:**
- ~70 lines removed from `dummysoftisp.cpp`
- Cleaner separation of concerns
- Reusable virtual camera logic

#### 2. Add Stub to Build System
**File**: `src/libcamera/meson.build`
```meson
libcamera_internal_headers += files([
  'internal/virtual_camera_stub.h',
])
```

#### 3. Test the Refactoring
```bash
meson setup build -Dsoftisp=enabled -Dpipelines='softisp,dummysoftisp'
meson compile -C build
export LD_LIBRARY_PATH=build/src/libcamera:build/src/ipa/dummysoftisp:$LD_LIBRARY_PATH
export SOFTISP_MODEL_DIR=/path/to/models
./build/tools/softisp-test-app --pipeline dummysoftisp --frames 5
```

---

### Future Enhancements (Priority 2)

#### 1. Add Test Pattern Generators
Create reusable frame generators:
```cpp
class TestPatternGenerator : public FrameGenerator {
    enum Pattern { GRADIENT, CHECKERBOARD, SOLID, NOISE };
    int generate(void *mem, const Size &size, uint32_t sequence) override;
};
```

#### 2. Create Full Virtual Camera Library
Expand the stub into a full library:
- `src/libcamera/internal/virtual_camera/`
  - `virtual_camera_base.h/cpp` - Base class with full features
  - `frame_generator.h` - Interface for frame generation
  - `test_pattern_generator.cpp` - Gradient, checkerboard, etc.
  - `image_frame_generator.cpp` - Load images from disk

#### 3. Add to Other Pipelines
Refactor other dummy/virtual pipelines to use the stub:
- `src/libcamera/pipeline/virtual/` (if simplified version needed)
- Future dummy pipelines

---

## Benefits Summary

### Code Reduction
| Component | Before | After | Reduction |
|-----------|--------|-------|-----------|
| `dummysoftisp` CameraData | ~100 lines | ~30 lines | **70%** |
| Thread Management | Manual | Automatic | **100%** |
| Request Queue | Manual | Automatic | **100%** |

### Maintainability
- ✅ Single source of truth for virtual camera logic
- ✅ Easy to fix bugs (one place to update)
- ✅ Easy to extend (override specific methods)

### Reusability
- ✅ Can be used by any pipeline
- ✅ No dependencies on specific hardware
- ✅ Header-only (zero compile-time cost)

---

## Risk Assessment

### Low Risk
- ✅ Stub is header-only (no linking issues)
- ✅ Backward compatible (old code still works)
- ✅ Easy to revert if needed
- ✅ Minimal changes to existing pipeline

### Mitigation
- Keep original `run()` logic as fallback
- Test thoroughly with existing test suite
- Gradual rollout (start with `dummysoftisp`)

---

## Success Criteria

The feature is complete when:
- [ ] `dummysoftisp` uses `VirtualCameraStub`
- [ ] Build compiles without errors
- [ ] All existing tests pass
- [ ] `softisp-test-app` works with dummy pipeline
- [ ] Code review approved
- [ ] Documentation updated

---

## Timeline

| Task | Estimated Time | Status |
|------|----------------|--------|
| Update `dummysoftisp` headers | 30 min | ⏳ Pending |
| Update `dummysoftisp` implementation | 1 hour | ⏳ Pending |
| Update build system | 10 min | ⏳ Pending |
| Compile and test | 30 min | ⏳ Pending |
| Fix any issues | 30 min | ⏳ Pending |
| **Total** | **~2.5 hours** | |

---

## References

- **Stub File**: `src/libcamera/internal/virtual_camera_stub.h`
- **Refactoring Guide**: `docs/REFACTOR_DUMMYSOFTISP_WITH_STUB.md`
- **Design Document**: `docs/DECOUPLING_VIRTUAL_CAMERA.md`
- **Transplant Report**: `VIRTUAL_PIPELINE_TRANSPLANT_REPORT.md`

---

## Notes

- The stub is **ready to use** - no additional implementation needed
- All documentation is complete with code examples
- The refactoring is **low-risk** and **reversible**
- This is a **foundational improvement** that will benefit future development

**Ready to start implementation?** The stub is created, documented, and waiting to be integrated!
