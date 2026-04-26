# libcamera Pipeline Code Logic Flow Documentation

## Overview

This document describes the complete logic flow of the libcamera pipeline system, with a focus on the simple pipeline handler and the Auto-Focus (AF) algorithm integration. The documentation covers the entire processing chain from camera initialization through frame processing and AF control.

## 1. Pipeline Handler Architecture

### 1.1 Core Pipeline Components

The libcamera pipeline consists of several key components that work together to process camera frames:

1. **PipelineHandler**: Base class for all pipeline implementations
2. **Camera**: Represents a physical camera device
3. **Request**: A capture request containing buffers and controls
4. **IPA (Image Processing Algorithm)**: Algorithms that process image data
5. **Frame Processing**: The complete flow from sensor to application

### 1.2 Pipeline Handler Lifecycle

```
1. Device Enumeration → Match → Camera Registration → Configuration → Start
2. Request Queueing → Processing → Buffer Completion → Request Completion
3. Stop → Cleanup
```

## 2. Simple Pipeline Handler Flow

### 2.1 Initialization and Device Matching

The SimplePipelineHandler follows this initialization sequence:

1. **match()**: Called by CameraManager to find compatible devices
   - Enumerates camera sensors and video nodes
   - Creates camera data instances for each valid path
   - Registers cameras with the camera manager

2. **init()**: Camera data initialization
   - Finds shortest path from sensor to video node using BFS
   - Initializes sensor controls and delayed controls handling
   - Sets up frame processing components

### 2.2 Configuration Phase

When an application configures the camera:

1. **generateConfiguration()**: Creates default stream configurations
2. **configure()**: Applies the requested configuration
   - Selects optimal sensor resolution and format
   - Configures format conversion/scaling if needed
   - Initializes IPA algorithms

### 2.3 Stream Processing Flow

During active streaming:

1. **start()**: Begins capture processing
2. **queueRequest()**: Queues capture requests
3. **queueRequestDevice()**: Sends request to hardware
4. **Buffer completion**: Hardware signals buffer completion
5. **completeBuffer()**: Notifies application of buffer completion
6. **completeRequest()**: Completes the entire request
7. **stop()**: Stops capture and cancels pending requests

## 3. IPA Algorithm Integration

### 3.1 IPA Module Structure

The IPA system is designed as a modular processing pipeline:

```
Application
    ↓
ControlList (AF Mode, Lens Position, etc.)
    ↓
IPA Algorithm Wrapper (Af.cpp)
    ↓
AfAlgo Core Algorithm (af_algo.cpp)
    ↓
AfStatsCalculator (af_stats.cpp)
    ↓
Raw Frame Data
```

### 3.2 AF Algorithm Components

#### AfAlgo (Core Algorithm)
- Implements the control loop for autofocus
- Supports three modes: Manual, Auto (one-shot), Continuous
- Uses hill climbing for CDAF and phase detection for PDAF
- Manages lens position in dioptres and maps to hardware units

#### AfStatsCalculator (Statistics)
- Calculates contrast metrics from raw image data
- Supports Sobel, Laplacian, and Variance methods
- Handles ROI (Region of Interest) processing
- Provides normalized contrast scores (0.0-1.0)

#### Af IPA Algorithm (Pipeline Integration)
- Bridges the IPA pipeline with the core AF algorithm
- Processes ControlList inputs
- Outputs lens position controls
- Coordinates with statistics calculator

### 3.3 AF Processing Flow

```
Frame Capture
    ↓
Raw Image Data → AfStatsCalculator → Contrast Metrics
    ↓
AfAlgo.process(contrast, phase, confidence)
    ↓
Lens Position Update (if needed)
    ↓
ControlList.set(controls::LensPosition, position)
    ↓
VCM Driver (Hardware Control)
```

## 4. Detailed Processing Steps

### 4.1 Request Flow

1. **Application queues request** with desired controls
2. **PipelineHandler.queueRequest()** adds request to waiting queue
3. **Request preparation** ensures buffers are ready
4. **doQueueRequests()** processes waiting requests
5. **queueRequestDevice()** sends to hardware
6. **Hardware processes** the request
7. **Buffer completion signals** trigger completeBuffer()
8. **Request completion** triggers completeRequest()

### 4.2 AF Algorithm Flow

1. **Control Handling**:
   - AfAlgo.handleControls() processes incoming ControlList
   - Updates mode, range, speed parameters if changed

2. **Frame Processing**:
   - Af.process() called for each frame
   - Retrieves contrast metrics from SwIspStats
   - Calls AfAlgo.process() with metrics

3. **Algorithm Processing**:
   - AfAlgo.process() runs control loop
   - Implements hill climbing or phase detection
   - Updates lens position when needed

4. **Output Generation**:
   - Lens position control updated in metadata
   - AF state reported in metadata

### 4.3 Configuration Flow

1. **IPA Configuration**:
   - IPASoftSimple.configure() sets up algorithms
   - Loads tuning data from YAML files
   - Initializes algorithm parameters

2. **Camera Configuration**:
   - SimpleCameraConfiguration.validate() validates stream configurations
   - Selects optimal sensor format/size
   - Configures format conversion if needed

## 5. Control Flow Diagram

```
┌─────────────────────┐
│   Application        │
│ (sets AfMode,        │
│  gets LensPosition)  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│   ControlList        │
│ (AfMode, LensPos,    │
│  AfState, etc.)      │
└─────────────────────┘
           │
           ▼
┌─────────────────────┐
│  Af IPA Algorithm    │
│ (af.cpp)             │
│ - Handles Controls   │
│ - Coordinates Algo  │
│ - Outputs Controls   │
└─────────────────────┘
           │
           ▼
┌─────────────────────┐
│   AfAlgo            │
│ (af_algo.cpp)       │
│ - Control loop       │
│ - Mode management   │
│ - PDAF+CDAF hybrid   │
└─────────────────────┘
           │
           ▼
┌─────────────────────┐
│ AfStatsCalculator   │
│ (af_stats.cpp)      │
│ - Contrast calc      │
│ - ROI support        │
│ - Multiple methods   │
└─────────────────────┘
           │
           ▼
┌─────────────────────┐
│  Raw Frame Data      │
│ (from camera sensor)│
└─────────────────────┘
```

## 6. Key Data Structures

### 6.1 ControlList
Holds camera controls that flow through the pipeline:
- AfMode (Manual, Auto, Continuous)
- LensPosition (normalized 0.0-1.0)
- AfState (Inactive, Scanning, Focusing, Failed)

### 6.2 AfStats
Contains focus metrics calculated from image data:
- contrast (normalized 0.0-1.0)
- phaseError (in pixels)
- phaseConfidence (0.0-1.0)
- rawContrast (unnormalized)
- validPixels (count)

### 6.3 IPAContext
Holds the processing context for IPA algorithms:
- Sensor information
- Configuration parameters
- Frame contexts
- Active state data

## 7. Integration Points

### 7.1 Hardware Integration
The AF algorithm is designed to be hardware-agnostic:
- Lens position mapped to VCM driver commands
- Statistics calculated on CPU from raw frames
- Controls exposed through standard ControlList interface

### 7.2 Software ISP Integration
The simple pipeline can use Software ISP for:
- Debayering raw sensor data
- Calculating AF statistics
- Format conversion and scaling

### 7.3 Pipeline Handler Integration
The SimplePipelineHandler integrates with IPA through:
- Request processing callbacks
- Buffer completion notifications
- ControlList processing in the capture flow

## 8. Current Limitations and Future Work

### 8.1 Frame Data Access
Currently the AF implementation uses placeholder contrast values. To be fully functional:
- Connect AfStatsCalculator to actual frame data
- Modify process() to receive raw frame pointer
- Or calculate stats in separate pass before process()

### 8.2 PDAF Integration
To support phase detection autofocus:
- Define PDAF statistics structure
- Calculate phase error from PDAF pixels
- Pass phase data to AfAlgo::process()

### 8.3 VCM Driver Interface
For hardware control:
- Map controls::LensPosition to VCM driver
- Handle VCM calibration data
- Add VCM-specific controls

This documentation provides a comprehensive overview of the pipeline code logic flow, focusing on how the AF algorithm integrates into the simple pipeline handler architecture.