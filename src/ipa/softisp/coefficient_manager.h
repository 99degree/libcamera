/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * CoefficientManager - Layer between algo.onnx and applier.onnx
 * Handles user overrides and rule-based modifications
 */
#pragma once

#include <string>
#include <vector>
#include <cstring>

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
    
    // LCS (Local Contrast Stretch) parameters
    float lcsStrength;
    float lcsThreshold;
    float lcsRadius;
    
    // AF (Auto Focus) parameters
    float afScore;
    int afRegionX;
    int afRegionY;
    int afRegionWidth;
    int afRegionHeight;
    bool afInFocus;
    float afDistance;
    
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
    bool overrideLcs;
    bool overrideAf;
    
    ISPCoefficients() {
        memset(awbGains, 0, sizeof(awbGains));
        memset(ccm, 0, sizeof(ccm));
        memset(tonemapCurve, 0, sizeof(tonemapCurve));
        gammaValue = 1.0f;
        memset(rgb2yuvMatrix, 0, sizeof(rgb2yuvMatrix));
        chromaSubsampleScale = 1.0f;
        
        // LCS defaults
        lcsStrength = 1.0f;
        lcsThreshold = 0.1f;
        lcsRadius = 5.0f;
        
        // AF defaults
        afScore = 0.0f;
        afRegionX = 0;
        afRegionY = 0;
        afRegionWidth = 0;
        afRegionHeight = 0;
        afInFocus = false;
        afDistance = 0.0f;
        
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
        overrideLcs = false;
        overrideAf = false;
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
     * Set LCS (Local Contrast Stretch) parameters
     */
    void setLcsParameters(float strength, float threshold, float radius);
    
    /**
     * Set AF (Auto Focus) parameters
     */
    void setAfParameters(float score, int regionX, int regionY, 
                         int regionWidth, int regionHeight,
                         bool inFocus, float distance);
    
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

} // namespace ipa::soft
} // namespace libcamera
