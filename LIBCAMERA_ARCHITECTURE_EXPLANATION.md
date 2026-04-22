# Libcamera Architecture: Pipeline vs. IPA Explained

## The Question
> "If all algorithms like LCS (Local Contrast Stretch), AF (Auto Focus), etc. are moved to IPA, then what does the pipeline do?"

## Short Answer
The **Pipeline Handler** is the **hardware-specific controller** that:
1. Manages physical camera hardware (sensors, ISPs, memory)
2. Handles buffer allocation and DMA
3. Controls V4L2 device streaming
4. Coordinates timing and synchronization
5. Routes requests between hardware and IPA

The **IPA (Image Processing Algorithm)** module is the **hardware-agnostic image processor** that:
1. Runs image processing algorithms (AWB, AE, LCS, etc.)
2. Generates ISP coefficients
3. Processes statistics from frames
4. Returns metadata/controls

## Detailed Architecture

### Layer 1: Application Layer
```
┌─────────────────────────────────────────────────────────────┐
│ libcamera-apps (libcamera-vid, libcamera-still, etc.)      │
│ Your custom application                                     │
└────────────────────┬────────────────────────────────────────┘
                     │
                     ▼
```

### Layer 2: Pipeline Handler (Hardware-Specific)
```
┌─────────────────────────────────────────────────────────────┐
│ Pipeline Handler (e.g., softisp, rkisp1, mali-c55, IPU3)   │
│                                                             │
│ Responsibilities:                                           │
│ ├── Hardware Enumeration (match())                          │
│ ├── Camera Configuration (configure())                      │
│ ├── Buffer Allocation (exportFrameBuffers())                │
│ ├── V4L2 Device Control (start/stop)                        │
│ ├── Request Queueing (queueRequestDevice())                 │
│ ├── DMA Buffer Management                                   │
│ ├── Memory Mapping (mmap)                                   │
│ ├── Interrupt Handling                                      │
│ ├── Timing/Synchronization                                  │
│ └── IPA Coordination                                        │
│                                                             │
│ Hardware-Specific Tasks:                                    │
│ ├── Opens /dev/videoX devices                               │
│ ├── Configures sensor registers                             │
│ ├── Sets up DMA engines                                     │
│ ├── Manages hardware ISP blocks                             │
│ └── Handles frame completion interrupts                     │
└────────────────────┬────────────────────────────────────────┘
                     │
                     │ Calls IPA methods
                     ▼
```

### Layer 3: IPA Module (Hardware-Agnostic)
```
┌─────────────────────────────────────────────────────────────┐
│ IPA Module (e.g., SoftIsp, Agc, Awb, Ccm, Lsc)             │
│                                                             │
│ Responsibilities:                                           │
│ ├── Image Statistics Processing                             │
│ ├── Algorithm Execution:                                    │
│ │   ├── AWB (Auto White Balance)                            │
│ │   ├── AE (Auto Exposure)                                  │
│ │   ├── AGC (Auto Gain Control)                             │
│ │   ├── LCS (Local Contrast Stretch)                        │
│ │   ├── AF (Auto Focus - statistics only)                   │
│ │   ├── CCM (Color Correction Matrix)                       │
│ │   └── Denoise, Sharpen, etc.                              │
│ ├── Coefficient Generation                                  │
│ ├── Control List Management                                 │
│ └── Metadata Generation                                     │
│                                                             │
│ Hardware-Agnostic:                                          │
│ ├── No knowledge of specific hardware                       │
│ ├── Works with any pipeline                                 │
│ ├── Pure algorithm implementation                           │
│ └── Can be CPU, GPU, or hardware accelerated                │
└─────────────────────────────────────────────────────────────┘
```

## Why This Separation?

### 1. **Hardware Abstraction**
- **Pipeline**: Knows about specific hardware (Rockchip RKISP, Raspberry Pi VC4, Intel IPU3)
- **IPA**: Doesn't care what hardware runs it

### 2. **Reusability**
- Same IPA module can work with multiple pipelines
- Example: `ipa_softisp.so` works with both `softisp` and `dummysoftisp` pipelines

### 3. **Modularity**
- Swap algorithms without changing hardware code
- Update hardware drivers without touching algorithms

### 4. **Testing**
- Test IPA algorithms independently of hardware
- Test pipeline with dummy IPA

## Real-World Example: Frame Processing Flow

```
1. Application calls camera->queueRequest(request)
   │
   ▼
2. Pipeline Handler (softisp.cpp)
   ├── queueRequestDevice() called
   ├── Gets frame buffer from request
   ├── Submits buffer to V4L2 hardware
   └── Returns immediately
   │
   ▼
3. Hardware captures frame (sensor → ISP → memory)
   │
   ▼
4. Hardware generates statistics (histogram, AWB, etc.)
   │
   ▼
5. Pipeline Handler receives frame completion
   ├── Extracts statistics from buffer
   └── Calls ipa_->processStats(frame, bufferId, controls)
   │
   ▼
6. IPA Module (softisp.cpp - SoftIsp class)
   ├── processStats() called
   ├── Runs algo.onnx (statistics → coefficients)
   ├── CoefficientManager applies overrides/rules
   ├── Runs applier.onnx (coefficients → gains)
   └── Returns ControlList with AWB gains, exposure, etc.
   │
   ▼
7. Pipeline Handler receives ControlList
   ├── Applies gains to hardware ISP
   ├── Updates sensor exposure settings
   └── Completes request with metadata
   │
   ▼
8. Application receives completed request
   └── Gets processed frame + metadata
```

## What About LCS, AF, and Other Algorithms?

### LCS (Local Contrast Stretch)
- **IPA Module**: Calculates contrast enhancement coefficients from statistics
- **Pipeline**: Applies the coefficients to the hardware ISP or software pipeline

### AF (Auto Focus)
- **IPA Module**: Analyzes focus metrics from image data
- **Pipeline**: Moves lens motor based on AF scores

### AE (Auto Exposure)
- **IPA Module**: Calculates target exposure from histogram
- **Pipeline**: Sets sensor exposure time and analog gain

### AWB (Auto White Balance)
- **IPA Module**: Calculates color gains from color statistics
- **Pipeline**: Applies gains to ISP white balance block

## The "Simple" Pipeline Exception

The `simple` pipeline is special:
- It's a **software-only** pipeline (no hardware)
- It **hardcodes** some algorithms internally
- It **can** load external IPA modules (like our SoftISP)
- Used for testing and platforms without dedicated ISP

## Our SoftISP Implementation

### Pipeline Handler (`src/libcamera/pipeline/softisp/`)
```cpp
// What it does:
- match(): Checks for /dev/video0 (real camera)
- configure(): Opens V4L2 device, sets format
- exportFrameBuffers(): Allocates DMA buffers
- start(): Starts V4L2 streaming
- queueRequestDevice(): Submits buffer to hardware
- processRequest(): Calls IPA processStats()
```

### IPA Module (`src/ipa/softisp/`)
```cpp
// What it does:
- init(): Loads algo.onnx and applier.onnx
- processStats(): 
  1. Runs algo.onnx (statistics → coefficients)
  2. CoefficientManager applies user overrides
  3. Runs applier.onnx (coefficients → gains)
  4. Returns AWB gains, exposure settings, etc.
```

## If All Algorithms Move to IPA...

### What the Pipeline Still Does:
1. **Hardware Control**
   - Opens/closes V4L2 devices
   - Configures sensor registers
   - Manages clock frequencies
   - Handles power management

2. **Buffer Management**
   - Allocates DMA-capable memory
   - Maps buffers for CPU/GPU access
   - Handles cache coherency
   - Manages buffer queues

3. **Streaming Control**
   - Starts/stops video streaming
   - Handles frame synchronization
   - Manages interrupt handlers
   - Processes frame completion events

4. **Request Coordination**
   - Queues requests to hardware
   - Waits for hardware completion
   - Extracts statistics from buffers
   - Routes metadata to application

5. **Hardware-Specific Features**
   - Sensor-specific controls
   - ISP block configuration
   - DMA engine setup
   - Memory layout optimization

### What the IPA Does:
1. **Algorithm Execution**
   - AWB, AE, AGC, LCS, AF statistics processing
   - Coefficient calculation
   - Neural network inference (our ONNX models)

2. **Decision Making**
   - What exposure settings to use
   - What white balance gains to apply
   - How much contrast enhancement
   - Focus score calculation

## Analogy

Think of it like a **car**:

- **Pipeline** = The car chassis, engine, transmission, wheels
  - Knows how to move the car
  - Handles the physical mechanics
  - Specific to car model (BMW vs. Toyota)

- **IPA** = The driver
  - Decides when to accelerate/brake
  - Steers based on road conditions
  - Can drive any car (hardware-agnostic)
  - Uses algorithms (experience, rules, AI)

The driver (IPA) makes decisions, but needs the car (Pipeline) to actually move.

## Summary

| Aspect | Pipeline Handler | IPA Module |
|--------|-----------------|------------|
| **Purpose** | Hardware control | Image processing |
| **Scope** | Hardware-specific | Hardware-agnostic |
| **Knows about** | V4L2, DMA, sensors | Algorithms, statistics |
| **Examples** | softisp, rkisp1, IPU3 | SoftIsp, Awb, Agc, Lsc |
| **Can be swapped?** | No (per platform) | Yes (interchangeable) |
| **Runs on** | CPU (control path) | CPU/GPU/Hardware |
| **Main tasks** | Buffer mgmt, streaming | Coefficient generation |

**Bottom Line**: Even if all algorithms move to IPA, the pipeline is still essential for:
- Controlling physical hardware
- Managing memory/buffers
- Handling streaming/interrupts
- Coordinating the overall camera system

The pipeline is the **bridge** between hardware and algorithms.
