/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - implementation of the ONNX-based Image Processing Algorithm.
 */
#include "softisp.h"
#include "af_algo.h"
#include "af_controls.h"
#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>
#include <onnxruntime_cxx_api.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace libcamera {
LOG_DEFINE_CATEGORY(SoftIsp)

namespace ipa::soft {

/* -----------------------------------------------------------------
 * SoftIsp::Impl - Private implementation holding ONNX sessions.
 * ----------------------------------------------------------------- */
struct SoftIsp::Impl {
    Impl() = default;
    ~Impl() = default;

    // ONNX Runtime environment and sessions
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "SoftIsp"};
    Ort::SessionOptions sessionOptions;
    std::unique_ptr<Ort::Session> algoSession;
    std::unique_ptr<Ort::Session> applierSession;
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    Ort::AllocatorWithDefaultOptions allocator;
    bool initialized = false;
    int imageWidth = 640;
    int imageHeight = 480;

    // Model paths
    std::string algoModelPath;
    std::string applierModelPath;

    // Input/Output names
    std::vector<const char*> algoInputNames;
    std::vector<const char*> algoOutputNames;
    std::vector<const char*> applierInputNames;
    std::vector<const char*> applierOutputNames;
};

/* -----------------------------------------------------------------
 * SoftIsp - Public Implementation
 * ----------------------------------------------------------------- */
SoftIsp::SoftIsp() : impl_(std::make_unique<Impl>())
{
}

SoftIsp::~SoftIsp() = default;

std::string SoftIsp::logPrefix() const
{
    return "SoftIsp";
}

int32_t SoftIsp::init(const IPASettings &settings, const SharedFD &fdStats, const SharedFD &fdParams,
                      const IPACameraSensorInfo &sensorInfo, const ControlInfoMap &sensorControls,
                      ControlInfoMap *ipaControls, bool *ccmEnabled)
{
    LOG(SoftIsp, Info) << "Initializing SoftISP algorithm";

    const char *modelDir = getenv("SOFTISP_MODEL_DIR");
    if (!modelDir) {
        LOG(SoftIsp, Error) << "SOFTISP_MODEL_DIR environment variable not set";
        return -EINVAL;
    }

    std::string algoPath = std::string(modelDir) + "/algo.onnx";
    std::string applierPath = std::string(modelDir) + "/applier.onnx";

    if (access(algoPath.c_str(), R_OK) != 0) {
        LOG(SoftIsp, Error) << "algo.onnx not found at " << algoPath;
        return -ENOENT;
    }

    try {
        impl_->algoSession = std::make_unique<Ort::Session>(impl_->env, algoPath.c_str(), impl_->sessionOptions);
        LOG(SoftIsp, Info) << "algo.onnx loaded successfully";
    } catch (const Ort::Exception &e) {
        LOG(SoftIsp, Error) << "Failed to load algo.onnx: " << e.what();
        return -EINVAL;
    }

    if (access(applierPath.c_str(), R_OK) != 0) {
        LOG(SoftIsp, Error) << "applier.onnx not found at " << applierPath;
        return -ENOENT;
    }

    try {
        impl_->applierSession = std::make_unique<Ort::Session>(impl_->env, applierPath.c_str(), impl_->sessionOptions);
        LOG(SoftIsp, Info) << "applier.onnx loaded successfully";
    } catch (const Ort::Exception &e) {
        LOG(SoftIsp, Error) << "Failed to load applier.onnx: " << e.what();
        return -EINVAL;
    }

    if (ccmEnabled)
        *ccmEnabled = true;

    LOG(SoftIsp, Info) << "SoftISP initialization complete";
    return 0;
}

int32_t SoftIsp::start()
{
    LOG(SoftIsp, Info) << "SoftISP algorithm started";
    return 0;
}

void SoftIsp::stop()
{
    LOG(SoftIsp, Info) << "SoftISP algorithm stopped";
}

int32_t SoftIsp::configure(const IPAConfigInfo &configInfo)
{
    LOG(SoftIsp, Info) << "Configuring SoftISP algorithm";
    impl_->imageWidth = configInfo.width;
    impl_->imageHeight = configInfo.height;
    return 0;
}

void SoftIsp::queueRequest(const uint32_t frame, const ControlList &sensorControls)
{
    LOG(SoftIsp, Debug) << "Queueing request for frame " << frame;
}

void SoftIsp::computeParams(const uint32_t frame)
{
    // Optional: Compute parameters if needed
}

/* -----------------------------------------------------------------
 * Public API for user overrides
 * ----------------------------------------------------------------- */
void SoftIsp::setAwbGains(float r, float g, float b)
{
    overrides_.awbGains[0] = r;
    overrides_.awbGains[1] = g;
    overrides_.awbGains[2] = b;
    overrides_.overrideAwbGains = true;
    LOG(SoftIsp, Info) << "AWB gains set: R=" << r << " G=" << g << " B=" << b;
}

void SoftIsp::setCcm(const float* matrix)
{
    if (matrix) {
        memcpy(overrides_.ccm, matrix, sizeof(overrides_.ccm));
        overrides_.overrideCcm = true;
        LOG(SoftIsp, Info) << "CCM matrix set";
    }
}

void SoftIsp::setGamma(float value)
{
    overrides_.gammaValue = value;
    overrides_.overrideGamma = true;
    LOG(SoftIsp, Info) << "Gamma set: " << value;
}

void SoftIsp::setTonemapCurve(const float* curve)
{
    if (curve) {
        memcpy(overrides_.tonemapCurve, curve, sizeof(overrides_.tonemapCurve));
        overrides_.overrideTonemap = true;
        LOG(SoftIsp, Info) << "Tone mapping curve set";
    }
}

void SoftIsp::setRgb2yuvMatrix(const float* matrix)
{
    if (matrix) {
        memcpy(overrides_.rgb2yuvMatrix, matrix, sizeof(overrides_.rgb2yuvMatrix));
        overrides_.overrideRgb2yuv = true;
        LOG(SoftIsp, Info) << "RGB to YUV matrix set";
    }
}

void SoftIsp::setChromaSubsampleScale(float scale)
{
    overrides_.chromaSubsampleScale = scale;
    overrides_.overrideChroma = true;
    LOG(SoftIsp, Info) << "Chroma subscale set: " << scale;
}

void SoftIsp::setLcsParameters(float strength, float threshold, float radius)
{
    overrides_.lcsStrength = strength;
    overrides_.lcsThreshold = threshold;
    overrides_.lcsRadius = radius;
    overrides_.overrideLcs = true;
    LOG(SoftIsp, Info) << "LCS parameters set: strength=" << strength 
                       << ", threshold=" << threshold << ", radius=" << radius;
}

void SoftIsp::setAfParameters(float score, int regionX, int regionY,
                              int regionWidth, int regionHeight,
                              bool inFocus, float distance)
{
    overrides_.afScore = score;
    overrides_.afRegionX = regionX;
    overrides_.afRegionY = regionY;
    overrides_.afRegionWidth = regionWidth;
    overrides_.afRegionHeight = regionHeight;
    overrides_.afInFocus = inFocus;
    overrides_.afDistance = distance;
    overrides_.overrideAf = true;
    LOG(SoftIsp, Info) << "AF parameters set: score=" << score 
                       << ", region=" << regionX << "," << regionY 
                       << "x" << regionWidth << "x" << regionHeight
                       << ", inFocus=" << inFocus << ", distance=" << distance;
}

void SoftIsp::clearOverrides()
{
    overrides_ = decltype(overrides_){};
    LOG(SoftIsp, Info) << "All overrides cleared";
}

void SoftIsp::handleAfControls(const ControlList &controls)
{
    afAlgo_.handleControls(controls);
}

void SoftIsp::applyOverrides(float* awbGains, float* ccm, float* tonemap, float* gamma,
                             float* rgb2yuv, float* chroma,
                             float* lcsStrength, float* lcsThreshold, float* lcsRadius,
                             float* afScore, int* afRegion, bool* afInFocus, float* afDistance)
{
    if (overrides_.overrideAwbGains && awbGains) {
        awbGains[0] = overrides_.awbGains[0];
        awbGains[1] = overrides_.awbGains[1];
        awbGains[2] = overrides_.awbGains[2];
    }
    if (overrides_.overrideCcm && ccm) {
        memcpy(ccm, overrides_.ccm, sizeof(overrides_.ccm));
    }
    if (overrides_.overrideTonemap && tonemap) {
        memcpy(tonemap, overrides_.tonemapCurve, sizeof(overrides_.tonemapCurve));
    }
    if (overrides_.overrideGamma && gamma) {
        *gamma = overrides_.gammaValue;
    }
    if (overrides_.overrideRgb2yuv && rgb2yuv) {
        memcpy(rgb2yuv, overrides_.rgb2yuvMatrix, sizeof(overrides_.rgb2yuvMatrix));
    }
    if (overrides_.overrideChroma && chroma) {
        *chroma = overrides_.chromaSubsampleScale;
    }
    if (overrides_.overrideLcs && lcsStrength && lcsThreshold && lcsRadius) {
        *lcsStrength = overrides_.lcsStrength;
        *lcsThreshold = overrides_.lcsThreshold;
        *lcsRadius = overrides_.lcsRadius;
    }
    if (overrides_.overrideAf && afScore && afRegion && afInFocus && afDistance) {
        *afScore = overrides_.afScore;
        afRegion[0] = overrides_.afRegionX;
        afRegion[1] = overrides_.afRegionY;
        afRegion[2] = overrides_.afRegionWidth;
        afRegion[3] = overrides_.afRegionHeight;
        *afInFocus = overrides_.afInFocus;
        *afDistance = overrides_.afDistance;
    }
}

int SoftIsp::loadConfig(const std::string& configPath)
{
    std::ifstream file(configPath);
    if (!file.is_open()) {
        LOG(SoftIsp, Error) << "Failed to open config file: " << configPath;
        return -ENOENT;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string key;
        iss >> key;
        
        if (key == "awb_gains") {
            float r, g, b;
            if (iss >> r >> g >> b) setAwbGains(r, g, b);
        } else if (key == "gamma") {
            float value;
            if (iss >> value) setGamma(value);
        } else if (key == "lcs_strength") {
            float s;
            if (iss >> s) {
                overrides_.lcsStrength = s;
                overrides_.overrideLcs = true;
            }
        } else if (key == "lcs_threshold") {
            float t;
            if (iss >> t) {
                overrides_.lcsThreshold = t;
                overrides_.overrideLcs = true;
            }
        } else if (key == "lcs_radius") {
            float r;
            if (iss >> r) {
                overrides_.lcsRadius = r;
                overrides_.overrideLcs = true;
            }
        } else if (key == "af_score") {
            float s;
            if (iss >> s) {
                overrides_.afScore = s;
                overrides_.overrideAf = true;
            }
        } else if (key == "af_in_focus") {
            int v;
            if (iss >> v) {
                overrides_.afInFocus = (v != 0);
                overrides_.overrideAf = true;
            }
        }
    }
    
    file.close();
    LOG(SoftIsp, Info) << "Configuration loaded from: " << configPath;
    return 0;
}

int SoftIsp::saveConfig(const std::string& configPath) const
{
    std::ofstream file(configPath);
    if (!file.is_open()) {
        LOG(SoftIsp, Error) << "Failed to create config file: " << configPath;
        return -EIO;
    }
    
    file << "# SoftISP Configuration\n";
    if (overrides_.overrideAwbGains) {
        file << "awb_gains " << overrides_.awbGains[0] << " " 
             << overrides_.awbGains[1] << " " << overrides_.awbGains[2] << "\n";
    }
    if (overrides_.overrideGamma) {
        file << "gamma " << overrides_.gammaValue << "\n";
    }
    if (overrides_.overrideLcs) {
        file << "lcs_strength " << overrides_.lcsStrength << "\n";
        file << "lcs_threshold " << overrides_.lcsThreshold << "\n";
        file << "lcs_radius " << overrides_.lcsRadius << "\n";
    }
    if (overrides_.overrideAf) {
        file << "af_score " << overrides_.afScore << "\n";
        file << "af_in_focus " << (overrides_.afInFocus ? 1 : 0) << "\n";
    }
    
    file.close();
    LOG(SoftIsp, Info) << "Configuration saved to: " << configPath;
    return 0;
}

/* -----------------------------------------------------------------
 * Main inference pipeline
 * ----------------------------------------------------------------- */
void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId, const ControlList &sensorControls)
{
    LOG(SoftIsp, Info) << ">>> processStats called for frame " << frame;

    // 1. Handle AF controls from ControlList (Mode, Range, Speed, etc.)
    handleAfControls(sensorControls);

    if (!impl_->initialized || !impl_->algoSession || !impl_->applierSession) {
        LOG(SoftIsp, Error) << "SoftISP not properly initialized";
        return;
    }

    try {
        // Prepare inputs for algo.onnx
        int imgWidth = impl_->imageWidth;
        int imgHeight = impl_->imageHeight;
        size_t imgSize = imgWidth * imgHeight;

        std::vector<int16_t> imageData(imgSize, 128);
        int64_t imgShape[] = {imgHeight, imgWidth};
        std::vector<int64_t> widthData(1, imgWidth);
        int64_t widthShape[] = {1};
        std::vector<int64_t> frameIdData(1, frame);
        int64_t frameIdShape[] = {1};
        std::vector<float> blackLevelData(1, 0.0f);
        int64_t blackLevelShape[] = {1};

        std::vector<Ort::Value> algoInputs;
        algoInputs.push_back(Ort::Value::CreateTensor(
            impl_->allocator, static_cast<void*>(imageData.data()),
            imageData.size() * sizeof(int16_t), imgShape, 2, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16));
        algoInputs.push_back(Ort::Value::CreateTensor(
            impl_->allocator, static_cast<void*>(widthData.data()),
            widthData.size() * sizeof(int64_t), widthShape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64));
        algoInputs.push_back(Ort::Value::CreateTensor(
            impl_->allocator, static_cast<void*>(frameIdData.data()),
            frameIdData.size() * sizeof(int64_t), frameIdShape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64));
        algoInputs.push_back(Ort::Value::CreateTensor(
            impl_->allocator, static_cast<void*>(blackLevelData.data()),
            blackLevelData.size() * sizeof(float), blackLevelShape, 1, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT));

        // Get input/output names
        std::vector<std::unique_ptr<char>> algoInputNamePtrs, algoOutputNamePtrs;
        std::vector<const char*> algoInputNames, algoOutputNames;
        for (size_t i = 0; i < 4; ++i) {
            auto name = impl_->algoSession->GetInputNameAllocated(i, impl_->allocator);
            algoInputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
            algoInputNames.push_back(algoInputNamePtrs.back().get());
        }
        for (size_t i = 0; i < impl_->algoSession->GetOutputCount(); ++i) {
            auto name = impl_->algoSession->GetOutputNameAllocated(i, impl_->allocator);
            algoOutputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
            algoOutputNames.push_back(algoOutputNamePtrs.back().get());
        }

        // Run algo.onnx
        LOG(SoftIsp, Debug) << "Running algo.onnx inference...";
        auto algoOutputs = impl_->algoSession->Run(
            Ort::RunOptions{nullptr}, algoInputNames.data(), algoInputs.data(),
            algoInputNames.size(), algoOutputNames.data(), algoOutputNames.size());
        LOG(SoftIsp, Info) << "algo.onnx completed: " << algoOutputs.size() << " outputs";

        // Extract coefficients
        float awbGains[3] = {1.0f, 1.0f, 1.0f};
        float ccm[9] = {1,0,0, 0,1,0, 0,0,1};
        float tonemap[16] = {0};
        float gamma = 2.2f;
        float rgb2yuv[9] = {0};
        float chroma = 1.0f;
        float lcsStrength = 1.0f, lcsThreshold = 0.1f, lcsRadius = 5.0f;
        float afScore = 0.0f, afDistance = 0.0f;
        int afRegion[4] = {0};
        bool afInFocus = false;

        if (algoOutputs.size() > 2) {
            auto* g = algoOutputs[2].GetTensorMutableData<float>();
            awbGains[0] = g[0]; awbGains[1] = g[1]; awbGains[2] = g[2];
        }
        if (algoOutputs.size() > 4) {
            auto* m = algoOutputs[4].GetTensorMutableData<float>();
            memcpy(ccm, m, std::min(sizeof(ccm), algoOutputs[4].GetTensorTypeAndShapeInfo().GetElementCount() * sizeof(float)));
        }
        if (algoOutputs.size() > 5) {
            auto* t = algoOutputs[5].GetTensorMutableData<float>();
            memcpy(tonemap, t, std::min(sizeof(tonemap), algoOutputs[5].GetTensorTypeAndShapeInfo().GetElementCount() * sizeof(float)));
        }
        if (algoOutputs.size() > 6) {
            auto* g = algoOutputs[6].GetTensorMutableData<float>();
            gamma = g[0];
        }
        if (algoOutputs.size() > 12) {
            auto* m = algoOutputs[12].GetTensorMutableData<float>();
            memcpy(rgb2yuv, m, std::min(sizeof(rgb2yuv), algoOutputs[12].GetTensorTypeAndShapeInfo().GetElementCount() * sizeof(float)));
        }
        if (algoOutputs.size() > 14) {
            auto* c = algoOutputs[14].GetTensorMutableData<float>();
            chroma = c[0];
        }

        // Apply user overrides
        applyOverrides(awbGains, ccm, tonemap, &gamma, rgb2yuv, &chroma,
                       &lcsStrength, &lcsThreshold, &lcsRadius,
                       &afScore, afRegion, &afInFocus, &afDistance);

        // Prepare applier.onnx inputs
        Ort::IoBinding ioBinding(*impl_->applierSession);
        ioBinding.BindInput(algoInputNames[0], algoInputs[0]);
        ioBinding.BindInput(algoInputNames[1], algoInputs[1]);
        ioBinding.BindInput(algoInputNames[2], algoInputs[2]);
        ioBinding.BindInput(algoInputNames[3], algoInputs[3]);

        struct CoeffMapping { size_t idx; const char* name; };
        CoeffMapping mappings[] = {
            {2, "awb.wb_gains.function"}, {4, "ccm.ccm.function"},
            {5, "tonemap.tonemap_curve.function"}, {6, "gamma.gamma_value.function"},
            {12, "yuv.rgb2yuv_matrix.function"}, {14, "chroma.subsample_scale.function"}
        };
        for (auto& m : mappings) {
            ioBinding.BindInput(m.name, std::move(algoOutputs[m.idx]));
        }

        std::vector<std::unique_ptr<char>> applierOutputNamePtrs;
        std::vector<const char*> applierOutputNames;
        for (size_t i = 0; i < impl_->applierSession->GetOutputCount(); ++i) {
            auto name = impl_->applierSession->GetOutputNameAllocated(i, impl_->allocator);
            applierOutputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
            applierOutputNames.push_back(applierOutputNamePtrs.back().get());
            ioBinding.BindOutput(applierOutputNames[i], impl_->memoryInfo);
        }

        // Run applier.onnx
        LOG(SoftIsp, Debug) << "Running applier.onnx inference...";
        impl_->applierSession->Run(Ort::RunOptions{nullptr}, ioBinding);
        auto applierOutputs = ioBinding.GetOutputValues();
        LOG(SoftIsp, Info) << "applier.onnx completed: " << applierOutputs.size() << " outputs";

        LOG(SoftIsp, Info) << "Frame " << frame << " processed successfully";

    } catch (const Ort::Exception& e) {
        LOG(SoftIsp, Error) << "ONNX inference failed: " << e.what();
    }
}

} // namespace ipa::soft
} // namespace libcamera
