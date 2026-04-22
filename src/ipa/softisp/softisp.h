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
#include <cstring>
#include <vector>

namespace libcamera {
namespace ipa::soft {

/**
 * ISP Coefficients structure holding all ISP parameters
 * This is the data structure passed between algo.onnx and applier.onnx
 */
struct ISPCoefficients {
    // AWB gains
    float awbGains[3]; // [R, G, B]
    
    // Color Correction Matrix (3x3)
    float ccm[9];
    
    // Tone mapping curve (simplified as control points)
    float tonemapCurve[16];
    
    // Gamma value
    float gammaValue;
    
    // RGB to YUV conversion matrix (3x3)
    float rgb2yuvMatrix[9];
    
    // Chroma subscale factor
    float chromaSubsampleScale;
    
    // Image dimensions
    int imageWidth;
    int imageHeight;
    
    // Frame ID
    int frameId;
    
    // Black level offset
    float blackLevelOffset;
    
    // User override flags
    bool overrideAwbGains;
    bool overrideCcm;
    bool overrideTonemap;
    bool overrideGamma;
    bool overrideRgb2yuv;
    bool overrideChroma;
    
    ISPCoefficients() {
        memset(awbGains, 0, sizeof(awbGains));
        memset(ccm, 0, sizeof(ccm));
        memset(tonemapCurve, 0, sizeof(tonemapCurve));
        gammaValue = 1.0f;
        memset(rgb2yuvMatrix, 0, sizeof(rgb2yuvMatrix));
        chromaSubsampleScale = 1.0f;
        imageWidth = 640;
        imageHeight = 480;
        frameId = 0;
        blackLevelOffset = 0.0f;
        overrideAwbGains = false;
        overrideCcm = false;
        overrideTonemap = false;
        overrideGamma = false;
        overrideRgb2yuv = false;
        overrideChroma = false;
    }
};

/**
 * CoefficientManager - Layer between algo.onnx and applier.onnx
 * Allows user overrides, rule-based modifications, and coefficient management
 */
class CoefficientManager {
public:
    CoefficientManager();
    ~CoefficientManager();
    
    /**
     * Set AWB gains override
     */
    void setAwbGains(float r, float g, float b);
    
    /**
     * Set CCM matrix override (3x3, row-major)
     */
    void setCcm(const float* matrix);
    
    /**
     * Set tone mapping curve override
     */
    void setTonemapCurve(const float* curve);
    
    /**
     * Set gamma value override
     */
    void setGamma(float value);
    
    /**
     * Set RGB to YUV matrix override (3x3, row-major)
     */
    void setRgb2yuvMatrix(const float* matrix);
    
    /**
     * Set chroma subscale override
     */
    void setChromaSubsampleScale(float scale);
    
    /**
     * Clear all overrides (use model outputs directly)
     */
    void clearOverrides();
    
    /**
     * Apply rules and modifications to coefficients
     * This is called after algo.onnx and before applier.onnx
     */
    void applyRules(ISPCoefficients* coeffs);
    
    /**
     * Get current coefficient state
     */
    const ISPCoefficients& getCurrentCoefficients() const { return currentCoeffs_; }
    
    /**
     * Load configuration from file (optional)
     */
    int loadConfig(const std::string& configPath);
    
    /**
     * Save current configuration to file
     */
    int saveConfig(const std::string& configPath) const;

private:
    ISPCoefficients currentCoeffs_;
    ISPCoefficients userOverrides_;
    
    // Rule-based adjustments
    bool enableAutoExposureAdjustment_;
    bool enableColorTemperatureCompensation_;
    float colorTemperatureK_;
    
    // LUT data (optional)
    std::vector<float> rgbLut_;
    bool hasRgbLut_;
    
    void applyColorTemperatureCompensation(ISPCoefficients* coeffs);
    void applyAutoExposureAdjustment(ISPCoefficients* coeffs);
    void applyUserOverrides(ISPCoefficients* coeffs);
};

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
    
    // Public API for user coefficient overrides
    void setAwbGains(float r, float g, float b);
    void setCcm(const float* matrix);
    void setGamma(float value);
    void setTonemapCurve(const float* curve);
    void setRgb2yuvMatrix(const float* matrix);
    void setChromaSubsampleScale(float scale);
    void clearOverrides();
    int loadConfig(const std::string& configPath);
    int saveConfig(const std::string& configPath) const;
    const ISPCoefficients& getCurrentCoefficients() const;

protected:
    std::string logPrefix() const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    CoefficientManager coeffManager_;
};

} // namespace ipa::soft
} // namespace libcamera
