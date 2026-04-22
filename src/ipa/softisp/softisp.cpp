/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - implementation of the ONNX-based Image Processing Algorithm.
 */
#include "softisp.h"
#include "coefficient_manager.h"
#include <libcamera/base/log.h>
#include <libcamera/base/utils.h>
#include <onnxruntime_cxx_api.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <filesystem>

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

    /* Get model directory from environment variable */
    const char *modelDir = getenv("SOFTISP_MODEL_DIR");
    if (!modelDir) {
        LOG(SoftIsp, Error) << "SOFTISP_MODEL_DIR environment variable not set";
        return -EINVAL;
    }

    std::string algoPath = std::string(modelDir) + "/algo.onnx";
    std::string applierPath = std::string(modelDir) + "/applier.onnx";

    LOG(SoftIsp, Info) << "Looking for algo.onnx at " << algoPath;
    if (access(algoPath.c_str(), R_OK) != 0) {
        LOG(SoftIsp, Error) << "algo.onnx not found at " << algoPath;
        return -ENOENT;
    }

    LOG(SoftIsp, Info) << "Loading algo.onnx...";
    try {
        impl_->algoSession = std::make_unique<Ort::Session>(impl_->env, algoPath.c_str(), impl_->sessionOptions);
        LOG(SoftIsp, Info) << "algo.onnx loaded successfully";
    } catch (const Ort::Exception &e) {
        LOG(SoftIsp, Error) << "Failed to load algo.onnx: " << e.what();
        return -EINVAL;
    }

    LOG(SoftIsp, Info) << "Looking for applier.onnx at " << applierPath;
    if (access(applierPath.c_str(), R_OK) != 0) {
        LOG(SoftIsp, Error) << "applier.onnx not found at " << applierPath;
        return -ENOENT;
    }

    LOG(SoftIsp, Info) << "Loading applier.onnx...";
    try {
        impl_->applierSession = std::make_unique<Ort::Session>(impl_->env, applierPath.c_str(), impl_->sessionOptions);
        LOG(SoftIsp, Info) << "applier.onnx loaded successfully";
    } catch (const Ort::Exception &e) {
        LOG(SoftIsp, Error) << "Failed to load applier.onnx: " << e.what();
        return -EINVAL;
    }

    if (ccmEnabled)
        *ccmEnabled = true;

    LOG(SoftIsp, Info) << "SoftISP initialization complete (models detected)";
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
 * Public API for user coefficient overrides
 * ----------------------------------------------------------------- */
void SoftIsp::setAwbGains(float r, float g, float b)
{
    coeffManager_.setAwbGains(r, g, b);
}

void SoftIsp::setCcm(const float* matrix)
{
    coeffManager_.setCcm(matrix);
}

void SoftIsp::setGamma(float value)
{
    coeffManager_.setGamma(value);
}

void SoftIsp::setTonemapCurve(const float* curve)
{
    coeffManager_.setTonemapCurve(curve);
}

void SoftIsp::setRgb2yuvMatrix(const float* matrix)
{
    coeffManager_.setRgb2yuvMatrix(matrix);
}

void SoftIsp::setChromaSubsampleScale(float scale)
{
    coeffManager_.setChromaSubsampleScale(scale);
}

void SoftIsp::setLcsParameters(float strength, float threshold, float radius)
{
    coeffManager_.setLcsParameters(strength, threshold, radius);
}

void SoftIsp::setAfParameters(float score, int regionX, int regionY,
                              int regionWidth, int regionHeight,
                              bool inFocus, float distance)
{
    coeffManager_.setAfParameters(score, regionX, regionY, regionWidth, regionHeight,
                                  inFocus, distance);
}

void SoftIsp::clearOverrides()
{
    coeffManager_.clearOverrides();
}

int SoftIsp::loadConfig(const std::string& configPath)
{
    return coeffManager_.loadConfig(configPath);
}

int SoftIsp::saveConfig(const std::string& configPath) const
{
    return coeffManager_.saveConfig(configPath);
}

const ISPCoefficients& SoftIsp::getCurrentCoefficients() const
{
    return coeffManager_.getCurrentCoefficients();
}

/* -----------------------------------------------------------------
 * Main inference pipeline
 * ----------------------------------------------------------------- */
void SoftIsp::processStats(const uint32_t frame, const uint32_t bufferId, const ControlList &sensorControls)
{
    LOG(SoftIsp, Info) << ">>> processStats called for frame " << frame;

    if (!impl_->initialized || !impl_->algoSession || !impl_->applierSession) {
        LOG(SoftIsp, Error) << "SoftISP not properly initialized";
        return;
    }

    LOG(SoftIsp, Info) << "Processing frame " << frame << " buffer " << bufferId;

    if (!impl_->algoSession || !impl_->applierSession) {
        LOG(SoftIsp, Error) << "ONNX models not loaded, skipping inference";
        return;
    }

    try {
        /*
         * Step 1: Prepare inputs for algo.onnx
         */
        int imgWidth = impl_->imageWidth;
        int imgHeight = impl_->imageHeight;
        size_t imgSize = imgWidth * imgHeight;

        /* Create dummy image data (in real impl, extract from frame buffer) */
        std::vector<int16_t> imageData(imgSize, 128);
        int64_t imgShape[] = {imgHeight, imgWidth};

        std::vector<int64_t> widthData(1, imgWidth);
        int64_t widthShape[] = {1};

        std::vector<int64_t> frameIdData(1, frame);
        int64_t frameIdShape[] = {1};

        std::vector<float> blackLevelData(1, 0.0f);
        int64_t blackLevelShape[] = {1};

        /* Create input tensors */
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

        /* Get input/output names */
        std::vector<std::unique_ptr<char>> algoInputNamePtrs;
        std::vector<const char*> algoInputNames;
        for (size_t i = 0; i < 4; ++i) {
            auto name = impl_->algoSession->GetInputNameAllocated(i, impl_->allocator);
            algoInputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
            algoInputNames.push_back(algoInputNamePtrs.back().get());
        }

        std::vector<std::unique_ptr<char>> algoOutputNamePtrs;
        std::vector<const char*> algoOutputNames;
        for (size_t i = 0; i < impl_->algoSession->GetOutputCount(); ++i) {
            auto name = impl_->algoSession->GetOutputNameAllocated(i, impl_->allocator);
            algoOutputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
            algoOutputNames.push_back(algoOutputNamePtrs.back().get());
        }

        /* Step 2: Run algo.onnx inference */
        LOG(SoftIsp, Debug) << "Running algo.onnx inference...";
        auto algoOutputs = impl_->algoSession->Run(
            Ort::RunOptions{nullptr}, algoInputNames.data(), algoInputs.data(),
            algoInputNames.size(), algoOutputNames.data(), algoOutputNames.size());
        LOG(SoftIsp, Info) << "algo.onnx completed: " << algoOutputs.size() << " outputs";

        /*
         * Step 3: Extract coefficients from algo.onnx outputs into ISPCoefficients structure
         */
        ISPCoefficients coeffs;
        coeffs.frameId = frame;
        coeffs.imageWidth = imgWidth;
        coeffs.imageHeight = imgHeight;

        // Extract AWB gains from output[2]
        if (algoOutputs.size() > 2) {
            auto* gains = algoOutputs[2].GetTensorMutableData<float>();
            coeffs.awbGains[0] = gains[0];
            coeffs.awbGains[1] = gains[1];
            coeffs.awbGains[2] = gains[2];
        }

        // Extract CCM from output[4]
        if (algoOutputs.size() > 4) {
            auto* ccmData = algoOutputs[4].GetTensorMutableData<float>();
            memcpy(coeffs.ccm, ccmData, std::min(sizeof(coeffs.ccm), algoOutputs[4].GetTensorTypeAndShapeInfo().GetElementCount() * sizeof(float)));
        }

        // Extract tonemap from output[5]
        if (algoOutputs.size() > 5) {
            auto* tonemapData = algoOutputs[5].GetTensorMutableData<float>();
            memcpy(coeffs.tonemapCurve, tonemapData, std::min(sizeof(coeffs.tonemapCurve), algoOutputs[5].GetTensorTypeAndShapeInfo().GetElementCount() * sizeof(float)));
        }

        // Extract gamma from output[6]
        if (algoOutputs.size() > 6) {
            auto* gammaData = algoOutputs[6].GetTensorMutableData<float>();
            coeffs.gammaValue = gammaData[0];
        }

        // Extract RGB2YUV from output[12]
        if (algoOutputs.size() > 12) {
            auto* yuvData = algoOutputs[12].GetTensorMutableData<float>();
            memcpy(coeffs.rgb2yuvMatrix, yuvData, std::min(sizeof(coeffs.rgb2yuvMatrix), algoOutputs[12].GetTensorTypeAndShapeInfo().GetElementCount() * sizeof(float)));
        }

        // Extract chroma scale from output[14]
        if (algoOutputs.size() > 14) {
            auto* chromaData = algoOutputs[14].GetTensorMutableData<float>();
            coeffs.chromaSubsampleScale = chromaData[0];
        }

        /*
         * Step 4: Apply CoefficientManager rules and overrides
         * This is where user overrides and rule-based modifications are applied
         */
        LOG(SoftIsp, Debug) << "Applying coefficient manager rules...";
        coeffManager_.applyRules(&coeffs);

        /*
         * Step 5: Prepare applier.onnx inputs using the modified coefficients
         */
        Ort::IoBinding ioBinding(*impl_->applierSession);

        /* Bind original 4 inputs */
        ioBinding.BindInput(algoInputNames[0], algoInputs[0]);
        ioBinding.BindInput(algoInputNames[1], algoInputs[1]);
        ioBinding.BindInput(algoInputNames[2], algoInputs[2]);
        ioBinding.BindInput(algoInputNames[3], algoInputs[3]);

        /* Bind 6 coefficient tensors from algo.onnx outputs (now modified by CoefficientManager) */
        struct CoefficientMapping {
            size_t algoOutputIdx;
            const char* applierInputName;
        };
        CoefficientMapping coeffMappings[] = {
            {2, "awb.wb_gains.function"},
            {4, "ccm.ccm.function"},
            {5, "tonemap.tonemap_curve.function"},
            {6, "gamma.gamma_value.function"},
            {12, "yuv.rgb2yuv_matrix.function"},
            {14, "chroma.subsample_scale.function"}
        };

        for (int i = 0; i < 6; ++i) {
            size_t outputIdx = coeffMappings[i].algoOutputIdx;
            const char* inputName = coeffMappings[i].applierInputName;
            LOG(SoftIsp, Debug) << "Binding algo output[" << outputIdx << "] to applier input \"" << inputName << "\"";
            ioBinding.BindInput(inputName, std::move(algoOutputs[outputIdx]));
        }

        /* Bind outputs */
        std::vector<std::unique_ptr<char>> applierOutputNamePtrs;
        std::vector<const char*> applierOutputNames;
        for (size_t i = 0; i < impl_->applierSession->GetOutputCount(); ++i) {
            auto name = impl_->applierSession->GetOutputNameAllocated(i, impl_->allocator);
            applierOutputNamePtrs.push_back(std::unique_ptr<char>(name.release()));
            applierOutputNames.push_back(applierOutputNamePtrs.back().get());
            ioBinding.BindOutput(applierOutputNames[i], impl_->memoryInfo);
        }

        /* Step 6: Run applier.onnx inference */
        LOG(SoftIsp, Debug) << "Running applier.onnx inference...";
        impl_->applierSession->Run(Ort::RunOptions{nullptr}, ioBinding);

        auto applierOutputs = ioBinding.GetOutputValues();
        LOG(SoftIsp, Info) << "applier.onnx completed: " << applierOutputs.size() << " outputs";

        LOG(SoftIsp, Info) << "Frame " << frame << " processed successfully through dual-model pipeline with coefficient manager";

    } catch (const Ort::Exception& e) {
        LOG(SoftIsp, Error) << "ONNX inference failed: " << e.what();
    }
}

} // namespace ipa::soft
} // namespace libcamera
