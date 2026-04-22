# AF Algorithm Integration Plan: RPi Adaptation for SoftISP

## Goal
Extract the robust Auto-Focus (AF) control logic from the Raspberry Pi (`src/ipa/rpi/controller/`) codebase, refactor it into a **public, hardware-agnostic library** under `src/ipa/libipa/`, and integrate it into the **SoftISP** pipeline (`src/ipa/softisp/`) to support **PDAF (Phase Detection)** and **CDAF (Contrast Detection)** modes.

## Architecture Overview

### 1. The Split
The AF system is divided into two distinct layers:
1.  **Stat Extractor (The "Eyes")**: Runs in the **Pipeline** or **IPA**. Reads raw sensor data/ONNX outputs and calculates a **Focus Metric** (Contrast Score or Phase Error).
2.  **Control Loop (The "Brain")**: Runs in the **IPA**. Takes the Focus Metric and decides the **Lens Position** (VCM value).

### 2. Directory Structure
```text
src/ipa/
├── libipa/                  # <--- NEW: Public shared library
│   ├── af_algo/             # Adapted RPi AF Control Loop
│   │   ├── af_algo.h        # Public API (Mode, State, Config)
│   │   ├── af_algo.cpp      # Core logic (Hill-climbing, PDAF, Scan)
│   │   └── meson.build      # Build config
│   └── af_stats/            # (Optional) Generic stats extraction helpers
│       ├── focus_metric.h
│       └── focus_metric.cpp
│
├── softisp/                 # Our SoftISP Pipeline
│   ├── softisp.h
│   ├── softisp.cpp          # Orchestrator: Extracts stats -> Calls AfAlgo
│   ├── af_extractor.cpp     # NEW: Specific implementation for SoftISP stats
│   └── meson.build          # Links against libipa
│
└── rpi/                     # Original RPi code (Read-only reference)
    └── controller/
        └── rpi/af.cpp       # Source of truth for logic
```

## Implementation Steps

### Phase 1: Create `libipa` Shared Library
**Objective:** Decouple the RPi AF logic from the RPi Controller framework.

1.  **Create Directory:**
    ```bash
    mkdir -p src/ipa/libipa/af_algo
    ```

2.  **Extract & Refactor `Af` Class:**
    *   Copy logic from `src/ipa/rpi/controller/rpi/af.cpp` and `af.h`.
    *   **Remove Dependencies:**
        *   Remove `RPiController::Controller` base class.
        *   Remove `YamlObject` parsing (replace with simple C++ setters).
        *   Remove `StatisticsPtr` (replace with simple `float contrast, float phase, float conf` arguments).
    *   **Simplify State:**
        *   Keep `Mode` (Manual, Auto, Continuous).
        *   Keep `State` (Idle, Scanning, Focusing).
        *   Keep parameters (`stepCoarse`, `pdafGain`, `contrastRatio`, etc.).
    *   **Output:** Return `int32_t` (VCM position) and `bool` (position updated).

3.  **Define Public API (`af_algo.h`):**
    ```cpp
    namespace libipa {
    class AfAlgo {
    public:
        enum class Mode { Manual, Auto, Continuous };
        enum class State { Idle, Scanning, Focusing, Failed };

        AfAlgo();
        ~AfAlgo();

        // Configuration
        void setRange(float minDioptres, float maxDioptres, float defaultDioptres);
        void setSpeed(float stepCoarse, float stepFine, float maxSlew);
        void setMode(Mode mode);
        void setPdafParams(float gain, float squelch, float confThresh);

        // Main Loop
        // Inputs: contrast (0.0-1.0), phase (pixels), conf (0.0-1.0)
        // Output: true if new lens position is ready
        bool process(float contrast, float phase = 0.0f, float conf = 0.0f);

        // Get Results
        int32_t getLensPosition() const; // VCM value (0-1023)
        State getState() const;
        float getTargetDioptres() const;
    };
    }
    ```

4.  **Update `meson.build`:**
    *   Add `libipa` as a shared library target.
    *   Ensure it exports `AfAlgo` symbols.

### Phase 2: Implement Stat Extractor in SoftISP
**Objective:** Provide the "Eyes" for the AF algorithm.

1.  **Create `src/ipa/softisp/af_extractor.cpp`:**
    *   Implement `float extractContrast(const ControlList& stats)`
    *   Implement `float extractPhase(const ControlList& stats)` (if PDAF data available)
    *   **Fallback:** If no hardware stats, implement a simple software contrast metric (e.g., variance of high-pass filtered image patch).

2.  **Integration Point:**
    *   In `SoftIsp::processStats()`, call the extractor to get `contrast` and `phase`.

### Phase 3: Integrate `AfAlgo` into SoftISP
**Objective:** Connect the "Brain" to the "Eyes".

1.  **Update `src/ipa/softisp/softisp.h`:**
    *   Add `#include <libipa/af_algo/af_algo.h>`
    *   Add member: `libipa::AfAlgo afAlgo_;`

2.  **Update `src/ipa/softisp/softisp.cpp`:**
    *   In `processStats()`:
        ```cpp
        // 1. Extract Stats
        float contrast = afExtractor.extractContrast(sensorControls);
        float phase = afExtractor.extractPhase(sensorControls);
        float conf = 0.8f; // Default confidence if not available

        // 2. Run Control Loop
        bool newPos = afAlgo_.process(contrast, phase, conf);

        // 3. Output to Pipeline
        if (newPos) {
            int32_t vcm = afAlgo_.getLensPosition();
            result.set(softisp::controls::focusPosition, vcm);
        }
        ```

3.  **Update `meson.build` for SoftISP:**
    *   Add dependency: `dependency('libipa')`
    *   Add include path: `-I${libipa_inc}`

### Phase 4: Configuration & Tuning
**Objective:** Make the AF algorithm configurable.

1.  **Config File Support:**
    *   Add `loadAfConfig(const std::string& path)` to `AfAlgo`.
    *   Parse a simple text file (INI or key-value) for parameters:
        ```ini
        [af]
        focus_min = 0.0
        focus_max = 12.0
        step_coarse = 1.0
        step_fine = 0.25
        pdaf_gain = -0.02
        contrast_ratio = 0.75
        ```
2.  **API Exposure:**
    *   Expose `setAfMode()` and `setAfRange()` in `SoftIsp` public API.

## Build System Changes

### `meson.build` (Root)
```meson
# Add libipa subproject
libipa_lib = static_library('libipa',
  sources: [
    'src/ipa/libipa/af_algo/af_algo.cpp',
    # ... other libipa sources
  ],
  include_directories: include_directories('src/ipa/libipa'),
  install: true,
  install_dir: libcamera_lib_dir
)
```

### `src/ipa/softisp/meson.build`
```meson
softisp_deps = [
  dependency('onnxruntime'),
  dependency('libcamera'),
  libipa_lib, # Link against our new library
]

softisp_sources = [
  'softisp.cpp',
  'softisp_module.cpp',
  'af_extractor.cpp', # New file
]
```

## Testing Strategy

1.  **Unit Test `AfAlgo`:**
    *   Create a mock test that feeds synthetic contrast data (e.g., a parabola) to `process()`.
    *   Verify `getLensPosition()` converges to the peak.
    *   Verify PDAF logic corrects position based on `phase` input.

2.  **Integration Test (SoftISP):**
    *   Run `softisp-test-app` with `--pipeline dummysoftisp`.
    *   Simulate focus metrics via a test script.
    *   Verify `focusPosition` control is generated correctly.

3.  **Hardware Test:**
    *   Connect a VCM lens.
    *   Run `camera-test` with `AfMode=Auto`.
    *   Observe lens movement and focus convergence.

## Timeline & Milestones

| Milestone | Description | Status |
| :--- | :--- | :--- |
| **M1** | Create `src/ipa/libipa/` directory structure | ⬜ |
| **M2** | Extract & refactor `AfAlgo` class (remove RPi deps) | ⬜ |
| **M3** | Build `libipa` and verify unit tests | ⬜ |
| **M4** | Implement `AfExtractor` in SoftISP | ⬜ |
| **M5** | Integrate `AfAlgo` into `SoftIsp::processStats` | ⬜ |
| **M6** | Add config file support & API exposure | ⬜ |
| **M7** | End-to-End testing with VCM lens | ⬜ |

## Risks & Mitigations

| Risk | Impact | Mitigation |
| :--- | :--- | :--- |
| **RPi Logic Complexity** | High | Strip down to core logic (Hill-climbing + PDAF). Ignore advanced RPi features (IR detection) for now. |
| **Stats Format Mismatch** | Medium | Create a generic `FocusMetric` struct in `libipa` that abstracts hardware vs. software stats. |
| **Performance Overhead** | Low | `AfAlgo` is pure math (no heavy loops). Extraction is the bottleneck; optimize that separately. |
| **VCM Range Mismatch** | Medium | Add a `mapDioptresToVCM()` function in `AfAlgo` that can be tuned per lens. |

## Conclusion
By moving the AF logic to `src/ipa/libipa/`, we create a **reusable, public component** that can be used by SoftISP and any future IPA modules. This separates the **algorithm** (Brain) from the **hardware interface** (Eyes), making the system modular, testable, and easier to maintain.
