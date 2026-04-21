/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp - Image Processing Algorithm that runs two ONNX models.
 *
 * This file declares the SoftIsp class which implements
 * libcamera::IPA::Algorithm. It is registered under the name
 * "softisp" and can be used by any pipeline (e.g. the default
 * "simple" pipeline via an alias mapping).
 *
 * The heavy ONNX‑runtime work is hidden inside the private Impl
 * structure to keep the public API clean.
 *********************************************************************/

#pragma once

#include "algorithm.h"
#include <string>

namespace libcamera {

/**
 * SoftIsp : public Algorithm
 *
 * The class follows the libcamera IPA API:
 *   - configure()   : load ONNX models, parse config
 *   - prepare()     : optional per‑frame setup
 *   - process()     : run the two‑stage ONNX inference
 *   - reset()       : clean up resources
 *
 * All public interactions use only the documented IPA API
 * (IPAContext, IPAStatistics, ControlList). No private
 * libcamera internals are accessed.
 ********************************************************************/
class SoftIsp : public Algorithm
{
public:
    SoftIsp();
    ~SoftIsp() override;

    // Called once when the pipeline starts.
    int configure(IPAContext &ctx, const IPAConfigInfo &info) override;

    // Called before each frame – currently does nothing.
    void prepare(IPAContext &ctx, const uint32_t frame,
                 IPAFrameContext &frameContext) override {}

    // Called for every frame. This is where the ONNX inference happens.
    void process(IPAContext &ctx, const uint32_t frame,
                 IPAFrameContext &frameContext,
                 const IPAStatistics *stats,
                 ControlList &metadata) override;

    // Cleanup resources when the pipeline shuts down.
    void reset() override;

    // Register the algorithm under the canonical name "softisp".
    REGISTER_ALGORITHM(SoftIsp)

private:
    /* -------------------------------------------------------------
       Private implementation detail – holds all ONNX runtime objects.
       ------------------------------------------------------------- */
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/* -----------------------------------------------------------------
   Implementation declaration – defined in softisp.cpp.
   ----------------------------------------------------------------- */
class SoftIsp::Impl {
public:
    Impl();
    ~Impl();

    // Load both ONNX models (algo.onnx and applier.onnx) from a directory.
    int load_models(const std::string &model_dir);

    // Execute the two‑stage inference chain.
    //   stats  – raw AWB statistics from the pipeline
    //   metadata – where we write the resulting ISP controls
    // Returns 0 on success, negative error code on failure.
    int run_inference(const IPAStatistics *stats, ControlList &metadata);

    // Helper that creates an Ort::Value from a raw float buffer.
    Ort::Value make_tensor(
        const std::vector<float> &data,
        const std::vector<int64_t> &shape) const;

    // Cached ONNX objects and paths
    Ort::Env               env_;
    std::unique_ptr<Ort::Session> algo_session_;
    std::unique_ptr<Ort::Session> applier_session_;
    Ort::MemoryInfo        memory_info_;
    std::vector<const char*> algo_input_names_;
    std::vector<const char*> algo_output_names_;
    std::vector<const char*> applier_input_names_;
    std::vector<const char*> applier_output_names_;
    std::string            algo_input_name_;
    std::string            algo_output_name_;
    std::string            applier_input_name_;
    std::string            applier_output_name_;
    std::string            algo_model_path_;
    std::string            applier_model_path_;
};

} // namespace libcamera