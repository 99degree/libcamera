#include <onnxruntime_cxx_api.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <memory>

int main() {
    try {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "SoftISPFullTest");
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);

        // Load models
        std::string algo_path = "/data/data/com.termux/files/home/softisp_models/algo.onnx";
        std::string applier_path = "/data/data/com.termux/files/home/softisp_models/applier.onnx";
        
        std::cout << "Loading algo.onnx..." << std::endl;
        Ort::Session algo_session(env, algo_path.c_str(), session_options);
        std::cout << "Loading applier.onnx..." << std::endl;
        Ort::Session applier_session(env, applier_path.c_str(), session_options);
        
        // Get input/output names for algo.onnx using GetInputNameAllocated
        std::vector<std::unique_ptr<char, Ort::CharPtrMemoryDeleter>> algo_input_names_ptr;
        std::vector<std::unique_ptr<char, Ort::CharPtrMemoryDeleter>> algo_output_names_ptr;
        std::vector<const char*> algo_input_names;
        std::vector<const char*> algo_output_names;
        
        size_t algo_input_count = algo_session.GetInputCount();
        size_t algo_output_count = algo_session.GetOutputCount();
        
        for (size_t i = 0; i < algo_input_count; ++i) {
            auto name = std::unique_ptr<char, Ort::CharPtrMemoryDeleter>(
                algo_session.GetInputNameAllocated(i, Ort::MemoryAllocator::GetCpuMemoryAllocator()));
            algo_input_names_ptr.push_back(std::move(name));
            algo_input_names.push_back(algo_input_names_ptr.back().get());
        }
        for (size_t i = 0; i < algo_output_count; ++i) {
            auto name = std::unique_ptr<char, Ort::CharPtrMemoryDeleter>(
                algo_session.GetOutputNameAllocated(i, Ort::MemoryAllocator::GetCpuMemoryAllocator()));
            algo_output_names_ptr.push_back(std::move(name));
            algo_output_names.push_back(algo_output_names_ptr.back().get());
        }
        
        std::cout << "algo.onnx inputs: " << algo_input_count << std::endl;
        std::cout << "algo.onnx outputs: " << algo_output_count << std::endl;
        
        // Create dummy input for algo.onnx (first input is AWB statistics)
        std::vector<float> awb_stats(128, 1.0f);
        std::vector<int64_t> awb_shape = {1, 128};
        
        auto memory_info = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
            memory_info, awb_stats.data(), awb_stats.size(), awb_shape.data(), awb_shape.size());
        
        // Run algo.onnx inference
        std::cout << "Running algo.onnx inference..." << std::endl;
        std::vector<Ort::Value> algo_outputs = algo_session.Run(
            Ort::RunOptions{nullptr},
            algo_input_names.data(), &input_tensor, 1,
            algo_output_names.data(), algo_output_count
        );
        
        std::cout << "algo.onnx inference completed successfully!" << std::endl;
        
        // Get the first output (coefficients)
        float* coeff_data = algo_outputs[0].GetTensorMutableData<float>();
        size_t coeff_count = algo_outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
        std::cout << "First output has " << coeff_count << " elements" << std::endl;
        
        // Get input/output names for applier.onnx
        std::vector<std::unique_ptr<char, Ort::CharPtrMemoryDeleter>> applier_input_names_ptr;
        std::vector<std::unique_ptr<char, Ort::CharPtrMemoryDeleter>> applier_output_names_ptr;
        std::vector<const char*> applier_input_names;
        std::vector<const char*> applier_output_names;
        
        size_t applier_input_count = applier_session.GetInputCount();
        size_t applier_output_count = applier_session.GetOutputCount();
        
        for (size_t i = 0; i < applier_input_count; ++i) {
            auto name = std::unique_ptr<char, Ort::CharPtrMemoryDeleter>(
                applier_session.GetInputNameAllocated(i, Ort::MemoryAllocator::GetCpuMemoryAllocator()));
            applier_input_names_ptr.push_back(std::move(name));
            applier_input_names.push_back(applier_input_names_ptr.back().get());
        }
        for (size_t i = 0; i < applier_output_count; ++i) {
            auto name = std::unique_ptr<char, Ort::CharPtrMemoryDeleter>(
                applier_session.GetOutputNameAllocated(i, Ort::MemoryAllocator::GetCpuMemoryAllocator()));
            applier_output_names_ptr.push_back(std::move(name));
            applier_output_names.push_back(applier_output_names_ptr.back().get());
        }
        
        std::cout << "applier.onnx inputs: " << applier_input_count << std::endl;
        std::cout << "applier.onnx outputs: " << applier_output_count << std::endl;
        
        // Create dummy inputs for applier
        std::vector<Ort::Value> applier_inputs;
        for (size_t i = 0; i < applier_input_count; ++i) {
            std::vector<float> dummy_data(64, 0.5f);
            std::vector<int64_t> dummy_shape = {1, 64};
            auto input = Ort::Value::CreateTensor<float>(
                memory_info, dummy_data.data(), dummy_data.size(), dummy_shape.data(), dummy_shape.size());
            applier_inputs.push_back(std::move(input));
        }
        
        // Replace first input with actual coefficients from algo output
        std::vector<int64_t> coeff_shape = {1, static_cast<int64_t>(coeff_count)};
        applier_inputs[0] = Ort::Value::CreateTensor<float>(
            memory_info, coeff_data, coeff_count, coeff_shape.data(), coeff_shape.size());
        
        std::cout << "Running applier.onnx inference..." << std::endl;
        std::vector<Ort::Value> applier_outputs = applier_session.Run(
            Ort::RunOptions{nullptr},
            applier_input_names.data(), applier_inputs.data(), applier_inputs.size(),
            applier_output_names.data(), applier_output_count
        );
        
        std::cout << "applier.onnx inference completed successfully!" << std::endl;
        
        // Extract gains from first output
        float* gains_data = applier_outputs[0].GetTensorMutableData<float>();
        size_t gains_count = applier_outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
        
        if (gains_count >= 3) {
            std::cout << "Extracted AWB Gains:" << std::endl;
            std::cout << "  R: " << gains_data[0] << std::endl;
            std::cout << "  G: " << gains_data[1] << std::endl;
            std::cout << "  B: " << gains_data[2] << std::endl;
        }
        
        std::cout << "\nSoftISP two-stage inference test PASSED!" << std::endl;
        return 0;
        
    } catch (const Ort::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
