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

namespace libcamera {
namespace ipa::soft {

// Forward declarations
struct ISPCoefficients;
class CoefficientManager;

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
    
    // LCS (Local Contrast Stretch) API
    void setLcsParameters(float strength, float threshold, float radius);
    
    // AF (Auto Focus) API
    void setAfParameters(float score, int regionX, int regionY,
                         int regionWidth, int regionHeight,
                         bool inFocus, float distance);
    
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
