# SoftISP IPA TODO

## Overview
This document tracks the remaining work for the SoftISP Image Processing Algorithm (IPA) that uses ONNX Runtime.

## Completed Work
- [x] Project scaffolding (`src/ipa/softisp/` with header, source, meson.build)
- [x] ONNX Runtime dependency added to meson.build
- [x] Basic SoftIsp class skeleton and build integration
- [x] TODO comment added about include path

## Pending TODOs

### 1. Parse configuration for model paths
- [ ] Modify `SoftIsp::configure()` to read model path entries from `IPAConfigInfo`.
- [ ] Provide fallback to environment variable `SOFTISP_MODEL_DIR`.

### 2. Implement ONNX inference chain
- [ ] Prepare input tensor from zone statistics for `algo.onnx`.
- [ ] Execute `algo.onnx` and retrieve coefficient tensor.
- [ ] Prepare input tensor for `applier.onnx` (full‑resolution Bayer + coefficients).
- [ ] Execute `applier.onnx` and extract ISP parameters.
- [ ] Map output tensors to `ControlList` entries (AWB gains, CCM, etc.).

### 3. Properly convert `IPAStatistics` to `awbStats_` format
- [ ] Copy raw AWB statistics into `Accumulator` structures.
- [ ] Handle byte‑order and scaling factors.

### 4. Replace hard‑coded green gain
- [ ] Use output from `applier.onnx` for green gain instead of `1.0`.

### 5. Add graceful error handling & fallback paths
- [ ] Catch ONNX Runtime exceptions and fall back to classical AWB.
- [ ] Log clear error messages.

### 6. Expose ONNX runtime options (optional)
- [ ] Add Meson option to select execution provider (CPU/GPU).
- [ ] Add option to set intra‑op threads.

### 7. Update documentation
- [ ] Document usage of `SOFTISP_MODEL_DIR`.
- [ ] Provide example config snippets.

## Tracking Progress
- Use GitHub Issues to link individual TODO items.
- Update checkboxes in this file as work progresses.
- When all items are marked complete, consider tagging a stable release.