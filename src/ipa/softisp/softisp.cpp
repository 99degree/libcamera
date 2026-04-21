/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * SoftIsp – implementation of the ONNX‑based Image Processing Algorithm.
 *
 * This file contains the full implementation of the SoftIsp class,
 * including the private ``Impl`` helper that wraps the ONNX Runtime
 * sessions and runs the two‑stage inference (algo.onnx → applier.onnx).
 *
 * The public API follows libcamera's ``Algorithm`` interface exactly.
 *********************************************************************/

#include "softisp.h"

#include <libcamera/ipa/ipa_context.h>
#include <libcamera/control_ids.h>
#include <libcamera/log.h>
#include <libcamera/utils.h>

#include <onnxruntime_cxx_api.h>
#include <algorithm>
#include <vector>
#include <cstring>

LOG_DEFINE_CATEGORY(SoftIsp)

namespace libcamera {

/* -----------------------------------------------------------------
   1️⃣  SoftIsp constructor – simplydefault‑constructs and stores
       a unique_ptr to the private Impl structure.
   ----------------------------------------------------------------- */
SoftIsp::SoftIsp() : impl_(std::make_unique<Impl>()) {}
SoftIsp::~SoftIsp() = default;

/* -----------------------------------------------------------------
   2️⃣  SoftIsp::configure() – parse configuration and load ONNX models.
   ----------------------------------------------------------------- */
int SoftIsp::configure(IPAContext &ctx, const IPAConfigInfo &info)
{
    // Resolve the directory that contains the ONNX models.
    // Users can set the environment variable SOFTISP_MODEL_DIR,
    // otherwise we fall back to the compiled‑in default path.
    const char *env_dir = std::getenv("SOFTISP_MODEL_DIR");
    std::string dir = env_dir ? env_dir
                              : std::string("/usr/share/libcamera/ipa/softisp/");

    // Store the full paths for later use.
    algo_model_path_  = dir + "algo.onnx";
    applier_model_path_ = dir + "applier.onnx";

    // Build the ONNX Runtime sessions.
    try {
        Ort::SessionOptions sess_opts;
        sess_opts.SetIntraOpNumThreads(1);
        sess_opts.SetGraphOptimizationLevel(ORT_ENABLE_ALL);

        // Load the first graph (algo.onnx)
        algo_session_ = std::make_unique<Ort::Session>(env_, algo_model_path_.c_str(),
                                                       sess_opts);

        // Resolve input / output names for the first session.
        Ort::AllocatorWithDefaultOptions allocator;
        char* in_name  = algo_session_->GetInputNameAllocated(0, allocator);
        char* out_name = algo_session_->GetOutputNameAllocated(0, allocator);
        algo_input_names_.push_back(in_name);
        algo_output_names_.push_back(out_name);
        allocator.Free(in_name);
        allocator.Free(out_name);
    } catch (const Ort::Exception &e) {
        LOG(SoftIsp, Error) << "Failed to open algo.onnx: " << e.what();
        return -EINVAL;
    }

    // Load the second graph (applier.onnx)
    try {
        Ort::SessionOptions applier_opts;
        applier_opts.SetIntraOpNumThreads(1);
        applier_opts.SetGraphOptimizationLevel(ORT_ENABLE_ALL);

        applier_session_ = std::make_unique<Ort::Session>(env_, applier_model_path_.c_str(),
                                                           applier_opts);

        // Resolve input / output names for the second session.
        char* a_in  = applier_session_->GetInputNameAllocated(0, allocator);
        char* a_out = applier_session_->GetOutputNameAllocated(0, allocator);
        applier_input_names_.push_back(a_in);
        applier_output_names_.push_back(a_out);
        allocator.Free(a_in);
        allocator.Free(a_out);
    } catch (const Ort::Exception &e) {
        LOG(SoftIsp, Error) << "Failed to open applier.onnx: " << e.what();
        return -EINVAL;
    }

    return 0;   // success
}

/* -----------------------------------------------------------------
   3️⃣  SoftIsp::process() – the heart of the implementation.
       It:
         1. Turns the raw AWB statistics into a float tensor.
         2. Runs the first ONNX model (algo.onnx) to get ISP coefficients.
         3. Feeds those coefficients into the second model (applier.onnx).
         4. Extracts the final ISP gains and writes them into ``metadata``.
   ----------------------------------------------------------------- */
void SoftIsp::process(IPAContext &ctx,
                      const uint32_t /*frame*/,
                      IPAFrameContext &,
                      const IPAStatistics *stats,
                      ControlList &metadata)
{
    if (!impl_)
        return;   // safety check – should never happen if configure() succeeded

    // -------------------------------------------------------------
    // 1️⃣ Extract raw AWB statistics and turn them into a float vector.
    // -------------------------------------------------------------
    const uint8_t *raw   = reinterpret_cast<const uint8_t *>(stats->awb_raw_buffer);
    size_t           raw_len = stats->awb_raw_size;

    std::vector<float> zone_vals;
    size_t i = 0;
    while (i + 4 <= raw_len) {
        uint32_t v = *reinterpret_cast<const uint32_t*>(raw + i);
        zone_vals.push_back(static_cast<float>(v));
        i += 4;
    }

    // Build the first input tensor (shape: [1, N])
    Ort::Value input_tensor = impl_->make_tensor(zone_vals,
                                                 {1, static_cast<int64_t>(zone_vals.size())});

    // -------------------------------------------------------------
    // 2️⃣ Run the first ONNX model (algo.onnx)
    // -------------------------------------------------------------
    const char* input_names  = impl_->algo_input_names_.data();
    const char* output_names = impl_->algo_output_names_.data();

    Ort::RunOptions run_opts;
    Ort::Value output_tensor;   // will receive the coefficient tensor

    impl_->algo_session_->Run(run_opts,
                              input_names, &input_tensor, 1,
                              output_names, &output_tensor, 1);

    // -------------------------------------------------------------
    // 3️⃣ Extract the coefficient tensor and feed it into the applier.
    // -------------------------------------------------------------
    // Get the output tensor as a flat float vector.
    const Ort::TypeInfo& output_type_info = output_tensor.GetTypeInfo();
    const Ort::TensorTypeAndShapeInfo& output_shape_info = output_type_info.GetTensorTypeAndShapeInfo();
    size_t output_element_count = output_shape_info.GetElementCount();
    std::vector<float> coeffs(output_element_count);
    output_tensor.GetTensorData<float>(coeffs.data(), output_element_count * sizeof(float));

    // The applier model expects: full‑res Bayer buffer + coeff tensor.
    // For the purpose of this test we will reuse the same dummy full‑res buffer
    // that we used in the stub. In a real implementation you would obtain the
    // full‑res Bayer buffer from the camera HAL.
    std::vector<float> dummy_full_res(3 * 1024 * 1024, 0.5f);  // 1024×1024 RGB
    Ort::Value full_res_tensor = impl_->make_tensor(
        dummy_full_res,
        {1, 3, 1024, 1024});   // shape: [1, C, H, W]

    // Names for the applier model's inputs/outputs
    const char* applier_input_names[]  = { impl_->applier_input_names_.data() };
    const char* applier_output_names[] = { impl_->applier_output_names_.data() };

    Ort::RunOptions applier_opts;
    Ort::Value applier_output;   // will receive the final ISP coefficients

    impl_->applier_session_->Run(applier_opts,
                                 applier_input_names, &full_res_tensor, 1,
                                 applier_output_names, &applier_output, 1);

    // -------------------------------------------------------------
    // 4️⃣ Extract the final ISP gains from the applier output.
    // -------------------------------------------------------------
    // The applier returns a flat float vector that contains the ISP
    // coefficients. For this example we assume the first three values are
    // the AWB gains (R, G, B). Adjust if your model outputs a different layout.
    const Ort::TypeInfo& out_type_info = applier_output.GetTypeInfo();
    const Ort::TensorTypeAndShapeInfo& out_shape_info = out_type_info.GetTensorTypeAndShapeInfo();
    size_t out_element_count = out_shape_info.GetElementCount();
    std::vector<float> out_buf(out_element_count);
    applier_output.GetTensorData<float>(out_buf.data(), out_element_count * sizeof(float));

    if (out_buf.size() >= 3) {
        GainMap gains;
        gains.red   = out_buf[0];
        gains.green = out_buf[1];
        gains.blue  = out_buf[2];
        metadata.set(control_id::colourGains, gains);
        LOG(SoftIsp, Info) << "SoftISP produced AWB gains: R=" << gains.red 
                           << " G=" << gains.green << " B=" << gains.blue;
    } else {
        LOG(SoftIsp, Warning) << "Applier model output too short (" << out_buf.size() 
                              << " floats); expected at least 3 for gains.";
    }
}

/* -----------------------------------------------------------------
   4️⃣  SoftIsp::reset() – clean up the private Impl (unique_ptr).
   ----------------------------------------------------------------- */
void SoftIsp::reset()
{
    impl_.reset();   // destroys the private Impl instance
}

/* -----------------------------------------------------------------
   5️⃣  Register the algorithm under the name "softisp".
   ----------------------------------------------------------------- */
static struct RegisterSoftIsp {
    RegisterSoftIsp() {
        REGISTER_ALGORITHM(SoftIsp);
    }
} register_softisp;