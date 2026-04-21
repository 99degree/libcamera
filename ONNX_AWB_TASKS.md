# SoftISP IPA: Task Breakdown and ONNX Interface Study

## Part 1: Step-by-Step Plan (Small Tasks)

### Phase 0: Prerequisites
- [x] Install ONNX Runtime development package (libonnxruntime-dev or equivalent).
- [x] Verify libcamera build environment is working.

### Phase 1: Directory and Files
- [x] Create directory `src/ipa/softisp/`.
- [x] Create header file `src/ipa/softisp/softisp.h`.
- [x] Create source file `src/ipa/softisp/softisp.cpp`.

### Phase 2: Header File Details
- [x] Add necessary includes: `<libcamera/ipa/algorithm.h>`, `<onnxruntime/core/session/onnxruntime_cxx_api.h>`.
- [x] Declare class `SoftIsp : public libcamera::ipa::Algorithm`.
- [x] Private members:
    - `Ort::Env env_;`
    - `std::unique_ptr<Ort::Session> algo_session_;`
    - `std::unique_ptr<Ort::Session> applier_session_;`
    - `std::string algo_input_name_;`, `std::string algo_output_name_;`
    - `std::string applier_input_name_;`, `std::string applier_output_name_;`
    - `Ort::MemoryInfo memory_info_;`
    - Zone parameters: `uint32_t cellsPerZoneX_;`, `cellsPerZoneY_;`, `cellsPerZoneThreshold_;`
    - `std::string model_dir_;` (optional, configurable)
- [x] Public methods: constructor, destructor, `configure`, `prepare`, `process`.

### Phase 3: Source File Implementation
#### Constructor
- [x] Initialize `Ort::Env` with logging level and name.
- [x] Set up `memory_info_ = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);`
- [x] Load model files (algo.onnx and applier.onnx) from a directory (default or from env var).
- [x] For each model:
    - [x] Create `Ort::SessionOptions`, set intra-op threads if desired.
    - [x] Attempt to create session; catch exceptions and log error.
    - [x] Query input and output count; expect exactly one input and one output for simplicity.
    - [x] Retrieve input and output names via `session.GetInputNameAllocated` and `GetOutputNameAllocated`.
    - [x] Store names in member variables.

#### configure()
- [x] Parse configuration (if any) for model directory; fallback to environment variable or hardcoded default.
- [x] Set zone generation parameters (could be fixed or configurable). For now, copy values from Awb algorithm: e.g., `cellsPerZoneX_ = 32; cellsPerZoneY_ = 12; cellsPerZoneThreshold_ = 15;`
- [x] Return 0 on success.

#### prepare()
- [x] Possibly reserve memory for tensors if we know max zones. For simplicity, we can allocate per frame.
- [x] Return 0.

#### process()
- [ ] Step 1: Generate zones.
    - We need to replicate the zone generation from Awb algorithm. Since we inherit from Algorithm, we cannot directly call Awb::process. We'll copy the logic:
        - Call `clearAwbStats()` (need to see if this is accessible; if not, we may need to have our own stats array).
        - Actually, the Awb algorithm uses `awb_stats_` and `zones_` as members. We can duplicate those members in our class.
        - For simplicity, we will duplicate the zone generation code from Awb::process (up to filling `zones_` vector).
        - We'll need the statistics from the metadata: `context.activeState.awb.stats`.
        - We'll implement `clearAwbStats`, `generateAwbStats`, and `generateZones` as private helper methods (copied from Awb algorithm).
- [ ] Step 2: After we have `zones_` vector (each element is a triplet of float: R, G, B averages), we convert to tensor.
    - Determine number of zones: `num_zones = zones_.size()`.
    - Create a float array of size `1 * num_zones * 3` (batch=1, sequence=num_zones, features=3).
    - Fill array: for each zone i, offset = i*3, data[offset] = zones_[i].r, data[offset+1] = zones_[i].g, data[offset+2] = zones_[i].b.
- [ ] Step 3: Create Ort::Value tensor for algo input.
    - Use `Ort::Value::CreateTensor<float>(memory_info_, data_ptr, size, input_shape.data(), input_shape.size())`.
    - input_shape = {1, num_zones, 3} (int64_t array).
- [ ] Step 4: Run algo session.
    - `const char* input_names[] = {algo_input_name_.c_str()};`
    - `Ort::Value* inputs[] = {input_tensor};`
    - `auto algo_outputs = algo_session_->Run(Ort::RunOptions{nullptr}, input_names, inputs, 1, &algo_output_name_, 1);`
    - Assuming single output.
- [ ] Step 5: Extract algo output tensor data.
    - Get pointer to float data: `float* algo_out_ptr = algo_outputs[0].GetTensorMutableData<float>();`
    - Get shape: `auto algo_out_shape = algo_outputs[0].GetTensorTypeAndShapeInfo().GetShape();`
    - Expected shape: maybe [1, something] or [something]. We'll need to know from model. For now, assume it's [1, feature_size] or [feature_size].
- [ ] Step 6: Prepare input for applier session.
    - Depending on algo output shape, we may need to reshape or transpose to match applier input.
    - The test_full_pipeline.py shows that the output of algo.onnx is used directly as input to applier.onnx (after merging with camera data). However, in our case, we only have the algo output; we might need to treat it as the full input for applier.
    - We'll need to inspect the models to know exact shapes. For simplicity, we assume algo output is a 1D tensor of length N, and applier expects [1, N] or [N].
    - We'll create a tensor from algo output data with appropriate shape.
- [ ] Step 7: Run applier session similarly.
- [ ] Step 8: Extract gains from applier output.
    - Expect output tensor of shape [1, 3] or [3] containing [red_gain, green_gain, blue_gain].
    - Copy values to gains array.
- [ ] Step 9: Compute color temperature using grey world algorithm on zones_ (same as Awb algorithm).
    - Implement steps: sort zones by R/G and B/G, discard top/bottom 25%, compute mean of middle half for R and B, then compute temperature.
    - We can reuse the grey world computation from Awb algorithm (maybe extract a helper function).
- [ ] Step 10: Set gains and temperature in context.
    - `context.activeState.awb.gains = {red_gain, green_gain, blue_gain};`
    - `context.activeState.awb.temperatureK = temperature;`
    - Update metadata: `*context.activeState.metadata = ...` (need to see how Awb does it; likely sets colour_gains and colour_temperature).
- [ ] Step 11: Return 0 on success, or negative error code on failure.

### Phase 4: Build System
- [x] Add `src/ipa/softisp/softisp.cpp` to `meson.build` under `libcamera_ipa_sources`.
- [x] Add include directory: `src/ipa/softisp`.
- [x] Link against ONNX Runtime: `dependency('onnxruntime', required: false)` or similar.
- [x] Ensure C++11 is enabled.

### Phase 5: Configuration and Deployment
- [x] Decide on model location mechanism: environment variable (e.g., `SOFTISP_MODEL_DIR`) or IPA configuration.
- [x] Implement reading model dir from environment in constructor or configure.
- [ ] Add error handling: if models fail to load, log error and optionally fall back to grey world AWB (by setting gains to 1,1,1 and computing temperature from zones).
- [x] Add logging using libcamera logging framework (e.g., `LOG_SOFTISP` category).

### Phase 6: Testing
- [ ] Create a simple test program to load dummy ONNX models and run inference.
- [ ] Test with actual models provided by user.
- [ ] Verify that gains and temperature are reasonable.
- [ ] Test fallback behavior when models missing.

### Phase 7: Documentation
- [x] Update libcamera documentation to describe the new SoftISP IPA.
- [x] Note the requirement for two ONNX models and how to specify their location.
- [x] Mention the ONNX Runtime dependency.

## Part 2: Study of test_full_pipeline.py (ONNX Interface)

### Key Observations for ONNX Runtime Usage
1. **Session Creation**:
   - `ort.InferenceSession(model_path, providers=providers)`
   - Providers list: e.g., `["CPUExecutionProvider"]`
   - Use try-catch to handle loading errors.

2. **Inspecting Model I/O**:
   - `session.get_inputs()` returns list of input metadata objects.
   - Each input has `.name`, `.type`, `.shape` (shape may contain undefined dimensions denoted by 'None' or negative numbers).
   - Similarly for `session.get_outputs()`.

3. **Preparing Inputs**:
   - Input data must be numpy arrays with correct dtype and shape.
   - The example uses `filter_feed_for_inputs(feed_dict, input_meta)` to ensure only required inputs are provided.
   - `map_outs_to_named_dict(output_meta, outs)` converts raw output list to dict keyed by output name.

4. **Running Inference**:
   - `session.run(None, feed_dict)` runs with default options; returns list of output arrays in order of `session.get_outputs()`.
   - To get named outputs, you can specify output names: `session.run([output_name1, output_name2], feed_dict)`.

5. **Chaining Models**:
   - Output of first model can be used directly as input to second model if shapes match.
   - In the example, they merge outputs from two sources (camera and algo) for the ISP model.
   - For our case, we will chain: zones tensor -> algo.onnx -> intermediate tensor -> applier.onnx -> gains.

6. **Data Types**:
   - ONNX Runtime typically uses float32 for tensors; ensure numpy arrays are `np.float32`.
   - Shapes must match exactly; undefined dimensions can be any size (but must be consistent).

7. **Error Handling**:
   - Wrap session runs in try-catch to log exceptions.

### Application to Our C++ Implementation
- Use Ort::Env, Ort::Session, Ort::SessionOptions.
- Use `Session::GetInputCount()`, `GetInputNameAllocated`, `GetInputTypeInfo` to get shape and type.
- Use ` Ort::Value::CreateTensor` with memory info and data pointer.
- Use `Session::Run` with input names and pointers to Ort::Value inputs, output names, and output Ort::Value pointers.
- Extract output data via `GetTensorMutableData<T>()` and `GetTensorTypeAndShapeInfo().GetShape()`.
- Ensure data layout matches expectations (ONNX Runtime uses row-major order, same as C++ row-major by default).
- For chaining, we may need to copy or reshape data; but we can pass the output tensor directly as input to the next session if shapes match (no copy needed if we keep the Ort::Value alive).

### Additional Notes from the Python Example
- The example uses queues and threading for asynchronous pipeline; we may consider that later for performance.
- They log input/output shapes and values for debugging; we can do similar with libcamera logging.
- They handle dynamic shapes (e.g., batch size 1) by using the actual shape from the data.

## Next Steps
1. Complete the implementation of the zone generation in the process() method by replicating Awb::process logic up to zone generation.
2. Implement the ONNX inference chain (algo.onnx -> applier.onnx) including tensor preparation and execution.
3. Implement the color temperature computation from the zones using grey world algorithm steps.
4. Set the gains and metadata correctly in the context and ControlList.
5. Add error handling and fallback to grey world AWB when models fail to load or inference fails.
6. Test with actual ONNX models.
