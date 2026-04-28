/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "softisp.h"
#include <chrono>
#include <libcamera/base/log.h>
#include <sys/mman.h>
#include <unistd.h>
#include <cstdlib>

using namespace libcamera;
using namespace libcamera::ipa::soft;

void SoftIsp::processFrame(const uint32_t frame, uint32_t bufferId,
                           const SharedFD &bufferFd, int32_t planeIndexIn,
                           int32_t planeIndexOut, int32_t width, int32_t height,
                           const ControlList & /*results*/)
{
    auto _pf_start = std::chrono::high_resolution_clock::now();
    LOG(SoftIsp, Debug) << "[IPA-pF] ENTRY frame=" << frame;

    // Pre-declare variables
    size_t grayscaleSize = static_cast<size_t>(width) * static_cast<size_t>(height);
    size_t rgbSize = grayscaleSize * 3;
    void* inputPtr = MAP_FAILED;
    void* outputPtr = MAP_FAILED;
    Format targetFormat = Format::RGB;

    // Format selection via environment
    static bool firstCall = true;
    if (firstCall) {
        firstCall = false;
        const char* env = std::getenv("SOFTISP_FORMAT");
        if (env && std::string(env) == "YUV") {
            targetFormat = Format::YUV;
        }
        LOG(SoftIsp, Info) << "[IPA-pF] Format: " << (targetFormat == Format::YUV ? "YUV" : "RGB");
    }

    // Validation
    if (!impl_ || !impl_->initialized) {
        LOG(SoftIsp, Error) << "[IPA-pF] ERROR: IPA not ready";
        goto cleanup;
    }

    // Map input buffer
    inputPtr = mmap(nullptr, grayscaleSize, PROT_READ, MAP_SHARED, bufferFd.get(), planeIndexIn * 4096);
    if (inputPtr == MAP_FAILED) {
        LOG(SoftIsp, Error) << "[IPA-pF] Failed to map input buffer";
        goto cleanup;
    }

    // Map output buffer
    outputPtr = mmap(nullptr, rgbSize, PROT_WRITE, MAP_SHARED, bufferFd.get(), planeIndexOut * 4096);
    if (outputPtr == MAP_FAILED) {
        LOG(SoftIsp, Error) << "[IPA-pF] Failed to map output buffer";
        munmap(inputPtr, grayscaleSize);
        goto cleanup;
    }

    // ONNX Inference: applier.onnx
    if (impl_->applierEngine.isLoaded()) {
        LOG(SoftIsp, Debug) << "[IPA-pF] Running ONNX inference";
        auto _inf_start = std::chrono::high_resolution_clock::now();
        std::vector<float> inputs;
        
        // Image input: grayscale normalizing
        const uint8_t* pixels = static_cast<uint8_t*>(inputPtr);
        for (size_t i = 0; i < grayscaleSize; i++) {
            inputs.push_back(pixels[i] / 255.0f);
        }

        // ONNX scalar parameters
        inputs.push_back(static_cast<float>(width));      // image_desc.input.width
        inputs.push_back(static_cast<float>(frame));     // image_desc.input.frame_id
        inputs.push_back(0.05f);                         // blacklevel.offset

        // ONNX parameters from processStats
        // AWB gains
        if (!impl_->awbGains.empty()) {
            for (float g : impl_->awbGains) inputs.push_back(g);
        } else {
            inputs.insert(inputs.end(), {1.0f, 1.0f, 1.0f}); // neutral
        }

        // Color correction matrix
        if (!impl_->ccmMatrix.empty()) {
            for (float m : impl_->ccmMatrix) inputs.push_back(m);
        } else {
            inputs.insert(inputs.end(), {
                1.0f, 0.0f, 0.0f, 
                0.0f, 1.0f, 0.0f, 
                0.0f, 0.0f, 1.0f 
            }); // identity
        }

        // Tonemap and gamma
        inputs.push_back(impl_->tonemapCurve > 0 ? impl_->tonemapCurve : 1.0f);
        inputs.push_back(impl_->gammaValue > 0 ? impl_->gammaValue : 1.0f);

        // YUV conversion matrix
        if (!impl_->yuvMatrix.empty()) {
            for (float y : impl_->yuvMatrix) inputs.push_back(y);
        } else {
            inputs.insert(inputs.end(), {
                0.2126f, 0.7152f, 0.0722f,  // Rec.709 RGB→YUV
                -0.1146f, -0.3854f, 0.5f,
                0.5f, -0.4542f, -0.0458f
            });
        }

        // Chroma subsample factors
        if (!impl_->subsampleScale.empty()) {
            for (float s : impl_->subsampleScale) inputs.push_back(s);
        } else {
            inputs.insert(inputs.end(), {0.5f, 0.5f, 1.0f, 1.0f});
        }

        LOG(SoftIsp, Info) << "[IPA-pF] ONNX input: " << inputs.size() << " values";

        // Run inference
        std::vector<float> outputs;
        int ret = impl_->applierEngine.runInference(inputs, outputs);
        auto _inf_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - _inf_start).count();

        LOG(SoftIsp, Info) << "[IPA-pF] ONNX inference: "<< _inf_us << "us, ret=" << ret 
                          << ", outputs=" << outputs.size();

        // Apply ONNX output tensor
        if (ret == 0 && !outputs.empty()) {
            std::string outputName = (targetFormat == Format::YUV) ? 
                                   "chroma.applier.function" : "rgb.rgb_out.function";

            const auto &outInfo = impl_->applierEngine.getOutputInfo();
            if (outInfo.find(outputName) != outInfo.end()) {
                const auto& tensor = outInfo.at(outputName);
                size_t expectedValues = grayscaleSize * 3;
                
                if (tensor.elementCount == expectedValues && outputs.size() >= expectedValues) {
                    LOG(SoftIsp, Info) << "[IPA-pF] Applying " << outputName
                                      << " (" << expectedValues << " values)";
                    
                    uint8_t* out_data = static_cast<uint8_t*>(outputPtr);
                    for (size_t i = 0; i < expectedValues; i++) {
                        float value = outputs[i];
                        value = std::max(0.0f, std::min(1.0f, value)); // clamp 0→1
                        out_data[i] = static_cast<uint8_t>(value * 255);
                    }
                    
                    msync(outputPtr, rgbSize, MS_SYNC);
                }
            }
        }
    }

cleanup:
    if (inputPtr != MAP_FAILED) {
        munmap(inputPtr, grayscaleSize);
    }
    if (outputPtr != MAP_FAILED) {
        munmap(outputPtr, rgbSize);
    }

    LOG(SoftIsp, Info) << "[IPA-pF] emitting frameDone";
    frameDone.emit(frame, bufferId);

    auto us = std::chrono::duration_cast<std::chrono::microseconds>( 
        std::chrono::high_resolution_clock::now() - _pf_start).count();
    LOG(SoftIsp, Info) << "[IPA-pF] EXIT: frame=" << frame << " dur=" << us << "us";
}