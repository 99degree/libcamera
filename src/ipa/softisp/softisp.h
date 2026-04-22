/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - Image Processing Algorithm that runs two ONNX models.
 */
#pragma once

#include <onnxruntime_cxx_api.h>
#include "module.h"
#include <libcamera/ipa/soft_ipa_interface.h>
#include <libcamera/ipa/ipa_module_info.h>
#include <string>
#include <vector>

namespace libcamera {
namespace ipa::soft {

/**
 * SoftIsp - IPA Module implementation for SoftISP
 * Inherits from both the generated interface and Module base class
 */
class SoftIsp : public IPASoftInterface, public Module {
public:
    SoftIsp();
    ~SoftIsp() override;

    // IPASoftInterface methods (all pure virtual)
    int32_t init(const IPASettings &settings, const SharedFD &fdStats, const SharedFD &fdParams,
                 const IPACameraSensorInfo &sensorInfo, const ControlInfoMap &sensorControls,
                 ControlInfoMap *ipaControls, bool *ccmEnabled) override;
    int32_t start() override;
    void stop() override;
    int32_t configure(const IPAConfigInfo &configInfo) override;
    void queueRequest(const uint32_t frame, const ControlList &sensorControls) override;
    void computeParams(const uint32_t frame) override;
    void processStats(const uint32_t frame, const uint32_t bufferId, const ControlList &sensorControls) override;
    
    // Public API for user overrides (stored in internal state)
    void setAwbGains(float r, float g, float b);
    void setCcm(const float* matrix);
    void setGamma(float value);
    void setTonemapCurve(const float* curve);
    void setRgb2yuvMatrix(const float* matrix);
    void setChromaSubsampleScale(float scale);
    void setLcsParameters(float strength, float threshold, float radius);
    void setAfParameters(float score, int regionX, int regionY,
                         int regionWidth, int regionHeight,
                         bool inFocus, float distance);
    void clearOverrides();
    int loadConfig(const std::string& configPath);
    int saveConfig(const std::string& configPath) const;

protected:
    std::string logPrefix() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
	// Store last processed output for pipeline access
	std::vector<float> lastOutputData_;
	uint32_t lastOutputBufferId_ = 0;
    
    // Internal state for overrides (simplified from CoefficientManager)
    struct {
        float awbGains[3] = {0};
        float ccm[9] = {0};
        float tonemapCurve[16] = {0};
        float gammaValue = 1.0f;
        float rgb2yuvMatrix[9] = {0};
        float chromaSubsampleScale = 1.0f;
        float lcsStrength = 1.0f;
        float lcsThreshold = 0.1f;
        float lcsRadius = 5.0f;
        float afScore = 0.0f;
        int afRegionX = 0, afRegionY = 0, afRegionWidth = 0, afRegionHeight = 0;
        bool afInFocus = false;
        float afDistance = 0.0f;
        
        bool overrideAwbGains = false;
        bool overrideCcm = false;
        bool overrideTonemap = false;
        bool overrideGamma = false;
        bool overrideRgb2yuv = false;
        bool overrideChroma = false;
        bool overrideLcs = false;
        bool overrideAf = false;
    } overrides_;
    
    void applyOverrides(float* awbGains, float* ccm, float* tonemap, float* gamma,
                        float* rgb2yuv, float* chroma,
                        float* lcsStrength, float* lcsThreshold, float* lcsRadius,
                        float* afScore, int* afRegion, bool* afInFocus, float* afDistance);
};

} // namespace ipa::soft
} // namespace libcamera
