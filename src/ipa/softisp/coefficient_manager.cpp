/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * CoefficientManager - Layer between algo.onnx and applier.onnx
 * Handles user overrides and rule-based modifications
 */

#include "coefficient_manager.h"
#include <libcamera/base/log.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace libcamera {
LOG_DEFINE_CATEGORY(SoftIsp)

namespace ipa::soft {

/* -----------------------------------------------------------------
 * CoefficientManager Implementation
 * ----------------------------------------------------------------- */

CoefficientManager::CoefficientManager()
    : enableAutoExposureAdjustment_(false),
      enableColorTemperatureCompensation_(false),
      colorTemperatureK_(6500.0f),
      hasRgbLut_(false)
{
    LOG(SoftIsp, Debug) << "CoefficientManager created";
}

CoefficientManager::~CoefficientManager()
{
    LOG(SoftIsp, Debug) << "CoefficientManager destroyed";
}

void CoefficientManager::setAwbGains(float r, float g, float b)
{
    userOverrides_.awbGains[0] = r;
    userOverrides_.awbGains[1] = g;
    userOverrides_.awbGains[2] = b;
    userOverrides_.overrideAwbGains = true;
    LOG(SoftIsp, Info) << "AWB gains set: R=" << r << " G=" << g << " B=" << b;
}

void CoefficientManager::setCcm(const float* matrix)
{
    if (matrix) {
        memcpy(userOverrides_.ccm, matrix, sizeof(userOverrides_.ccm));
        userOverrides_.overrideCcm = true;
        LOG(SoftIsp, Info) << "CCM matrix set";
    }
}

void CoefficientManager::setTonemapCurve(const float* curve)
{
    if (curve) {
        memcpy(userOverrides_.tonemapCurve, curve, sizeof(userOverrides_.tonemapCurve));
        userOverrides_.overrideTonemap = true;
        LOG(SoftIsp, Info) << "Tone mapping curve set";
    }
}

void CoefficientManager::setGamma(float value)
{
    userOverrides_.gammaValue = value;
    userOverrides_.overrideGamma = true;
    LOG(SoftIsp, Info) << "Gamma set: " << value;
}

void CoefficientManager::setRgb2yuvMatrix(const float* matrix)
{
    if (matrix) {
        memcpy(userOverrides_.rgb2yuvMatrix, matrix, sizeof(userOverrides_.rgb2yuvMatrix));
        userOverrides_.overrideRgb2yuv = true;
        LOG(SoftIsp, Info) << "RGB to YUV matrix set";
    }
}

void CoefficientManager::setChromaSubsampleScale(float scale)
{
    userOverrides_.chromaSubsampleScale = scale;
    userOverrides_.overrideChroma = true;
    LOG(SoftIsp, Info) << "Chroma subscale set: " << scale;
}

void CoefficientManager::setLcsParameters(float strength, float threshold, float radius)
{
    userOverrides_.lcsStrength = strength;
    userOverrides_.lcsThreshold = threshold;
    userOverrides_.lcsRadius = radius;
    userOverrides_.overrideLcs = true;
    LOG(SoftIsp, Info) << "LCS parameters set: strength=" << strength 
                       << ", threshold=" << threshold << ", radius=" << radius;
}

void CoefficientManager::setAfParameters(float score, int regionX, int regionY,
                                         int regionWidth, int regionHeight,
                                         bool inFocus, float distance)
{
    userOverrides_.afScore = score;
    userOverrides_.afRegionX = regionX;
    userOverrides_.afRegionY = regionY;
    userOverrides_.afRegionWidth = regionWidth;
    userOverrides_.afRegionHeight = regionHeight;
    userOverrides_.afInFocus = inFocus;
    userOverrides_.afDistance = distance;
    userOverrides_.overrideAf = true;
    LOG(SoftIsp, Info) << "AF parameters set: score=" << score 
                       << ", region=" << regionX << "," << regionY 
                       << "x" << regionWidth << "x" << regionHeight
                       << ", inFocus=" << inFocus << ", distance=" << distance;
}

void CoefficientManager::clearOverrides()
{
    userOverrides_ = ISPCoefficients();
    LOG(SoftIsp, Info) << "All overrides cleared";
}

void CoefficientManager::applyUserOverrides(ISPCoefficients* coeffs)
{
    if (!coeffs) return;
    
    if (userOverrides_.overrideAwbGains) {
        coeffs->awbGains[0] = userOverrides_.awbGains[0];
        coeffs->awbGains[1] = userOverrides_.awbGains[1];
        coeffs->awbGains[2] = userOverrides_.awbGains[2];
        LOG(SoftIsp, Debug) << "Applied AWB gains override";
    }
    
    if (userOverrides_.overrideCcm) {
        memcpy(coeffs->ccm, userOverrides_.ccm, sizeof(coeffs->ccm));
        LOG(SoftIsp, Debug) << "Applied CCM override";
    }
    
    if (userOverrides_.overrideTonemap) {
        memcpy(coeffs->tonemapCurve, userOverrides_.tonemapCurve, sizeof(coeffs->tonemapCurve));
        LOG(SoftIsp, Debug) << "Applied tonemap override";
    }
    
    if (userOverrides_.overrideGamma) {
        coeffs->gammaValue = userOverrides_.gammaValue;
        LOG(SoftIsp, Debug) << "Applied gamma override: " << coeffs->gammaValue;
    }
    
    if (userOverrides_.overrideRgb2yuv) {
        memcpy(coeffs->rgb2yuvMatrix, userOverrides_.rgb2yuvMatrix, sizeof(coeffs->rgb2yuvMatrix));
        LOG(SoftIsp, Debug) << "Applied RGB2YUV override";
    }
    
    if (userOverrides_.overrideChroma) {
        coeffs->chromaSubsampleScale = userOverrides_.chromaSubsampleScale;
        LOG(SoftIsp, Debug) << "Applied chroma override";
    }
    
    if (userOverrides_.overrideLcs) {
        coeffs->lcsStrength = userOverrides_.lcsStrength;
        coeffs->lcsThreshold = userOverrides_.lcsThreshold;
        coeffs->lcsRadius = userOverrides_.lcsRadius;
        LOG(SoftIsp, Debug) << "Applied LCS override";
    }
    
    if (userOverrides_.overrideAf) {
        coeffs->afScore = userOverrides_.afScore;
        coeffs->afRegionX = userOverrides_.afRegionX;
        coeffs->afRegionY = userOverrides_.afRegionY;
        coeffs->afRegionWidth = userOverrides_.afRegionWidth;
        coeffs->afRegionHeight = userOverrides_.afRegionHeight;
        coeffs->afInFocus = userOverrides_.afInFocus;
        coeffs->afDistance = userOverrides_.afDistance;
        LOG(SoftIsp, Debug) << "Applied AF override";
    }
}

void CoefficientManager::applyColorTemperatureCompensation(ISPCoefficients* coeffs)
{
    if (!enableColorTemperatureCompensation_ || !coeffs) return;
    
    // Simple color temperature compensation
    // Adjust AWB gains based on target color temperature
    float targetTemp = colorTemperatureK_;
    
    // Normalize to 5000-10000K range
    float normTemp = (targetTemp - 5000.0f) / 5000.0f;
    
    // Adjust gains (simplified model)
    float rAdjust = 1.0f + 0.2f * (1.0f - normTemp);
    float bAdjust = 1.0f + 0.2f * normTemp;
    
    if (userOverrides_.overrideAwbGains) {
        coeffs->awbGains[0] *= rAdjust;
        coeffs->awbGains[2] *= bAdjust;
    }
    
    LOG(SoftIsp, Debug) << "Applied color temp compensation: " << targetTemp << "K";
}

void CoefficientManager::applyAutoExposureAdjustment(ISPCoefficients* coeffs)
{
    if (!enableAutoExposureAdjustment_ || !coeffs) return;
    
    // Auto-exposure adjustment based on scene brightness
    // This would typically use histogram data from the frame
    // For now, apply a simple gain adjustment
    
    float targetBrightness = 0.5f; // Mid-gray
    float currentBrightness = 0.5f; // Would come from statistics
    
    float exposureAdjust = targetBrightness / std::max(currentBrightness, 0.01f);
    exposureAdjust = std::min(std::max(exposureAdjust, 0.5f), 2.0f); // Clamp
    
    // Apply to gamma
    coeffs->gammaValue *= exposureAdjust;
    coeffs->gammaValue = std::min(std::max(coeffs->gammaValue, 0.5f), 2.5f);
    
    LOG(SoftIsp, Debug) << "Applied auto-exposure adjustment: " << exposureAdjust;
}

void CoefficientManager::applyRules(ISPCoefficients* coeffs)
{
    if (!coeffs) return;
    
    LOG(SoftIsp, Debug) << "Applying coefficient rules";
    
    // 1. Apply user overrides first
    applyUserOverrides(coeffs);
    
    // 2. Apply rule-based adjustments
    applyColorTemperatureCompensation(coeffs);
    applyAutoExposureAdjustment(coeffs);
    
    // 3. Apply LUT if available
    if (hasRgbLut_ && !rgbLut_.empty()) {
        // Apply RGB LUT to tonemap curve
        size_t lutSize = std::min(rgbLut_.size(), sizeof(coeffs->tonemapCurve) / sizeof(float));
        memcpy(coeffs->tonemapCurve, rgbLut_.data(), lutSize * sizeof(float));
        LOG(SoftIsp, Debug) << "Applied RGB LUT (" << lutSize << " points)";
    }
    
    // Store final coefficients
    currentCoeffs_ = *coeffs;
    
    LOG(SoftIsp, Debug) << "Coefficient rules applied successfully";
}

int CoefficientManager::loadConfig(const std::string& configPath)
{
    std::ifstream file(configPath);
    if (!file.is_open()) {
        LOG(SoftIsp, Error) << "Failed to open config file: " << configPath;
        return -ENOENT;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string key;
        iss >> key;
        
        if (key == "awb_gains") {
            float r, g, b;
            if (iss >> r >> g >> b) {
                setAwbGains(r, g, b);
            }
        } else if (key == "gamma") {
            float value;
            if (iss >> value) {
                setGamma(value);
            }
        } else if (key == "color_temp") {
            float temp;
            if (iss >> temp) {
                colorTemperatureK_ = temp;
                enableColorTemperatureCompensation_ = true;
            }
        } else if (key == "auto_exposure") {
            bool enable;
            if (iss >> enable) {
                enableAutoExposureAdjustment_ = enable;
            }
        } else if (key == "lcs_strength") {
            float strength;
            if (iss >> strength) {
                userOverrides_.lcsStrength = strength;
                userOverrides_.overrideLcs = true;
            }
        } else if (key == "lcs_threshold") {
            float threshold;
            if (iss >> threshold) {
                userOverrides_.lcsThreshold = threshold;
                userOverrides_.overrideLcs = true;
            }
        } else if (key == "lcs_radius") {
            float radius;
            if (iss >> radius) {
                userOverrides_.lcsRadius = radius;
                userOverrides_.overrideLcs = true;
            }
        } else if (key == "af_score") {
            float score;
            if (iss >> score) {
                userOverrides_.afScore = score;
                userOverrides_.overrideAf = true;
            }
        } else if (key == "af_in_focus") {
            bool inFocus;
            if (iss >> inFocus) {
                userOverrides_.afInFocus = inFocus;
                userOverrides_.overrideAf = true;
            }
        }
        // Add more config options as needed
    }
    
    file.close();
    LOG(SoftIsp, Info) << "Configuration loaded from: " << configPath;
    return 0;
}

int CoefficientManager::saveConfig(const std::string& configPath) const
{
    std::ofstream file(configPath);
    if (!file.is_open()) {
        LOG(SoftIsp, Error) << "Failed to create config file: " << configPath;
        return -EIO;
    }
    
    file << "# SoftISP Configuration\n";
    file << "# Generated automatically\n\n";
    
    if (userOverrides_.overrideAwbGains) {
        file << "awb_gains " 
             << userOverrides_.awbGains[0] << " "
             << userOverrides_.awbGains[1] << " "
             << userOverrides_.awbGains[2] << "\n";
    }
    
    if (userOverrides_.overrideGamma) {
        file << "gamma " << userOverrides_.gammaValue << "\n";
    }
    
    if (enableColorTemperatureCompensation_) {
        file << "color_temp " << colorTemperatureK_ << "\n";
    }
    
    if (enableAutoExposureAdjustment_) {
        file << "auto_exposure 1\n";
    }
    
    if (userOverrides_.overrideLcs) {
        file << "lcs_strength " << userOverrides_.lcsStrength << "\n";
        file << "lcs_threshold " << userOverrides_.lcsThreshold << "\n";
        file << "lcs_radius " << userOverrides_.lcsRadius << "\n";
    }
    
    if (userOverrides_.overrideAf) {
        file << "af_score " << userOverrides_.afScore << "\n";
        file << "af_in_focus " << (userOverrides_.afInFocus ? 1 : 0) << "\n";
    }
    
    file.close();
    LOG(SoftIsp, Info) << "Configuration saved to: " << configPath;
    return 0;
}

} // namespace ipa::soft
} // namespace libcamera
