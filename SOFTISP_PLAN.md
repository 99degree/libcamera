# SoftISP Algorithm for libcamera IPA

## Overview

This document describes the SoftISP algorithm for libcamera IPA that uses ONNX runtime to load two models (algo.onnx and applier.onnx) to create a complete software ISP pipeline. The algorithm processes Bayer statistics through a two-stage ONNX inference process:

1. **algo.onnx**: Takes Bayer zone statistics and generates ISP coefficients (AWB gains, CCM, lens shading, tone mapping, etc.)
2. **applier.onnx**: Takes the full-resolution Bayer image and the generated coefficients to produce the final processed image

This mirrors the structure of hardware ISPs where statistics processing and image application are separate stages.

## Design

The SoftISP algorithm replaces the existing `Awb` algorithm but extends its functionality significantly. While it reuses the existing framework for generating zones from AWB statistics (for the first stage), it replaces the simple gain computation with a full ONNX-based ISP pipeline.

### Processing Flow

1. **Zone Generation** (Same as Awb algorithm):
   - In `process`, we call `clearAwbStats`, `generateAwbStats`, and `generateZones` to fill the `zones_` vector (each zone is an RGB triple: R, G, B averages from Bayer statistics).

2. **Stage 1 - Coefficient Generation (algo.onnx)**:
   - Convert the `zones_` vector into a tensor suitable for algo.onnx
   - Run algo.onnx inference to generate a complete set of ISP coefficients
   - Extract and store these coefficients (AWB gains, CCM matrix, lens correction parameters, etc.) in the IPA context

3. **Stage 2 - Image Application (applier.onnx)**:
   - Prepare the full-resolution Bayer image buffer as input to applier.onnx
   - Run applier.onnx inference using both the image buffer and the coefficients from Stage 1
   - Output the final processed image (typically YUV or RGB) ready for display/encoding

4. **Metadata Generation**:
   - Compute color temperature from zones using grey world algorithm (for metadata/display)
   - Update metadata with colour gains, colour temperature, and other ISP parameters

## Implementation Details

### Header File (softisp.h)

We have created a class `SoftIsp` that inherits from `libcamera::ipa::Algorithm`.

We have included:
- <libcamera/ipa/algorithm.h>
- <onnxruntime/core/session/onnxruntime_cxx_api.h>

We have private members for:
- Ort::Env environment
- Ort::Session for algo_model (coefficient generation)
- Ort::Session for applier_model (image application)
- Ort::MemoryInfo for tensor allocation
- Input/output node names for both models (queried from the models)
- Zone statistics vector (R, G, B per zone in [0, 1] range)
- Zone generation parameters (cellsPerZoneX_, cellsPerZoneY_, cellsPerZoneThreshold_)
- Model file paths (configured via environment variable)

We have overridden:
- configure
- prepare
- process

### Source File (softisp.cpp)

In the constructor:
- We have initialized the ONNX runtime environment.
- We have loaded the two models (algo.onnx and applier.onnx) from a directory specified by the environment variable `SOFTISP_MODEL_DIR` with a fallback to `/usr/share/libcamera/ipa/softisp/`.

In configure:
- We have set up the cellsPerZoneX_, cellsPerZoneY_, and cellsPerZoneThreshold_ as in the existing Awb algorithm.
- We validate that the models have the expected input/output structure.

In process (to be implemented):
1. **Zone Generation**: Replicate Awb::process logic up to zone generation
   - Clear statistics buffers
   - Generate AWB statistics from Bayer data
   - Generate zones vector from statistics

2. **Stage 1 Inference (algo.onnx)**:
   - Convert zones vector to tensor [1, num_zones, 3]
   - Run algo.onnx session
   - Extract coefficient tensor (containing AWB gains, CCM, etc.)
   - Store coefficients in context for Stage 2 and metadata

3. **Stage 2 Preparation**:
   - Prepare full-resolution Bayer image as tensor input for applier.onnx
   - Combine image tensor with coefficients from Stage 1

4. **Stage 2 Inference (applier.onnx)**:
   - Run applier.onnx session
   - Extract final processed image tensor
   - Output the processed frame to libcamera pipeline

5. **Metadata Generation**:
   - Compute color temperature using grey world algorithm on zones
   - Set gains and temperature in context.activeState.awb
   - Update metadata with colour gains, colour temperature, and other ISP parameters

### Note on Green Gain
The existing Awb algorithm sets green gain to 1.0. Our model might output a different value. We will use the model output for all three gains, as the ISP context properly handles per-channel gains.

### Dependencies
We have linked against the ONNX runtime library in meson.build.

## Configuration
The user must provide the two ONNX models (algo.onnx and applier.onnx) in a known directory. We have made the path configurable via the environment variable `SOFTISP_MODEL_DIR`.

## Potential Issues & Future Enhancements
- **Latency**: ONNX inference may add significant latency. Consider frame skipping when system is busy.
- **Temporal Smoothing**: Add a rule-based manager between stages to limit drastic coefficient changes between frames.
- **Additional ISP Stages**: Future work could add LSC (Lens Shading Correction) and GDC (Geometry Distortion Correction) models.
- **Auto Focus Integration**: Use sharpness metrics from algo.onnx to drive VCM position via V4L2 register writes.
- **Performance**: Profile inference time and optimize using session options (execution providers, graph optimization).

## Next Steps
1. Complete the implementation of the zone generation in the process() method.
2. Implement the ONNX inference chain (algo.onnx -> applier.onnx) including tensor preparation for both stages.
3. Implement the color temperature computation from the zones.
4. Set the gains, metadata, and output frame correctly.
5. Add error handling and fallback to grey world AWB.
6. Test with actual ONNX models.
