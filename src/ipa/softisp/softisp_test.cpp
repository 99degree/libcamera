#include <onnxruntime_cxx_api.h>
#include <iostream>
#include <vector>
#include <fstream>

int main() {
    try {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "SoftISPTest");
        Ort::SessionOptions session_options;
        session_options.SetIntraOpNumThreads(1);
        session_options.SetGraphOptimizationLevel(ORT_ENABLE_ALL);

        // Load algo.onnx
        std::string algo_path = "/data/data/com.termux/files/home/softisp_models/algo.onnx";
        std::cout << "Loading algo.onnx from: " << algo_path << std::endl;
        Ort::Session algo_session(env, algo_path.c_str(), session_options);
        std::cout << "algo.onnx loaded successfully!" << std::endl;

        // Load applier.onnx
        std::string applier_path = "/data/data/com.termux/files/home/softisp_models/applier.onnx";
        std::cout << "Loading applier.onnx from: " << applier_path << std::endl;
        Ort::Session applier_session(env, applier_path.c_str(), session_options);
        std::cout << "applier.onnx loaded successfully!" << std::endl;

        // Get input/output names for algo.onnx
        size_t algo_input_count = algo_session.GetInputCount();
        size_t algo_output_count = algo_session.GetOutputCount();
        std::cout << "algo.onnx: " << algo_input_count << " inputs, " << algo_output_count << " outputs" << std::endl;

        // Get input/output names for applier.onnx
        size_t applier_input_count = applier_session.GetInputCount();
        size_t applier_output_count = applier_session.GetOutputCount();
        std::cout << "applier.onnx: " << applier_input_count << " inputs, " << applier_output_count << " outputs" << std::endl;

        std::cout << "SoftISP models loaded and ready for inference!" << std::endl;
        return 0;
    } catch (const Ort::Exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
