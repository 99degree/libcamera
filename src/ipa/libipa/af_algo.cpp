/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * \file af_algo.cpp
 * \brief Hardware-agnostic Auto-Focus Algorithm Implementation
 * 
 * Implementation of the AfAlgo class, adapted from the Raspberry Pi
 * controller AF algorithm. Supports both PDAF (Phase Detection) and
 * CDAF (Contrast Detection) autofocus methods.
 */

#include "af_algo.h"
#include "af_controls.h"

#include <fstream>
#include <sstream>
#include <cstring>

#include <libcamera/control_ids.h>

namespace libcamera {
LOG_DEFINE_CATEGORY(LibipaAf)

namespace libipa {

AfAlgo::AfAlgo()
    : mode_(AfMode::Manual),
      state_(AfState::Idle),
      ftarget_(1.0f),
      fsmooth_(1.0f),
      hwpos_(512),
      positionUpdated_(false),
      focusMin_(0.0f),
      focusMax_(12.0f),
      focusDefault_(1.0f),
      stepCoarse_(1.0f),
      stepFine_(0.25f),
      maxSlew_(2.0f),
      pdafGain_(-0.02f),
      pdafSquelch_(0.125f),
      pdafConfThresh_(0.1f),
      contrastRatio_(0.75f),
      confEpsilon_(8.0f),
      skipFrames_(5),
      frameCount_(0),
      lastContrast_(0.0f),
      scanDirection_(1.0f),
      scanStep_(stepCoarse_),
      retriggerDelay_(10),
      retriggerCount_(0)
{
    LOG(LibipaAf, Info) << "AfAlgo initialized";
}

AfAlgo::~AfAlgo() = default;

void AfAlgo::setRange(float minD, float maxD, float defaultD)
{
    focusMin_ = std::max(0.0f, minD);
    focusMax_ = std::max(focusMin_ + 0.1f, maxD);
    focusDefault_ = std::clamp(defaultD, focusMin_, focusMax_);
    ftarget_ = focusDefault_;
    fsmooth_ = focusDefault_;
    LOG(LibipaAf, Debug) << "AF Range set: " << focusMin_ << " - " << focusMax_ 
                         << " (default=" << focusDefault_ << ")";
}

void AfAlgo::setSpeed(float coarse, float fine, float slew)
{
    stepCoarse_ = std::max(0.1f, coarse);
    stepFine_ = std::max(0.05f, fine);
    maxSlew_ = std::max(0.5f, slew);
    LOG(LibipaAf, Debug) << "AF Speed set: coarse=" << stepCoarse_ 
                         << ", fine=" << stepFine_ << ", slew=" << maxSlew_;
}

void AfAlgo::setMode(AfMode mode)
{
    if (mode_ != mode) {
        mode_ = mode;
        switch (mode) {
        case AfMode::Manual:
            goIdle();
            LOG(LibipaAf, Info) << "AF Mode: Manual";
            break;
        case AfMode::Auto:
            startScan();
            LOG(LibipaAf, Info) << "AF Mode: Auto (Scan starting)";
            break;
        case AfMode::Continuous:
            state_ = AfState::Focusing;
            lastContrast_ = 0.0f;
            LOG(LibipaAf, Info) << "AF Mode: Continuous";
            break;
        }
    }
}

void AfAlgo::setPdafParams(float gain, float squelch, float confThresh)
{
    pdafGain_ = gain;
    pdafSquelch_ = squelch;
    pdafConfThresh_ = std::clamp(confThresh, 0.0f, 1.0f);
    LOG(LibipaAf, Debug) << "PDAF params: gain=" << gain << ", squelch=" << squelch 
                         << ", confThresh=" << confThresh;
}

void AfAlgo::setCdafParams(float ratio, float epsilon, uint32_t skipFrames)
{
    contrastRatio_ = std::clamp(ratio, 0.5f, 0.95f);
    confEpsilon_ = std::max(1.0f, epsilon);
    skipFrames_ = std::min(20u, std::max(1u, skipFrames));
    LOG(LibipaAf, Debug) << "CDAF params: ratio=" << ratio << ", epsilon=" << epsilon 
                         << ", skipFrames=" << skipFrames;
}

int AfAlgo::loadConfig(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        LOG(LibipaAf, Error) << "Failed to open config: " << path;
        return -ENOENT;
    }

    std::string line;
    bool inAfSection = false;
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        
        // Check for section header
        if (line[0] == '[') {
            inAfSection = (line.find("af]") != std::string::npos || 
                          line.find("AF]") != std::string::npos);
            continue;
        }
        
        if (!inAfSection) continue;

        std::istringstream iss(line);
        std::string key;
        iss >> key;

        if (key == "focus_min") {
            float val;
            if (iss >> val) setRange(val, focusMax_, focusDefault_);
        } else if (key == "focus_max") {
            float val;
            if (iss >> val) setRange(focusMin_, val, focusDefault_);
        } else if (key == "focus_default") {
            float val;
            if (iss >> val) setRange(focusMin_, focusMax_, val);
        } else if (key == "step_coarse") {
            float val;
            if (iss >> val) stepCoarse_ = val;
        } else if (key == "step_fine") {
            float val;
            if (iss >> val) stepFine_ = val;
        } else if (key == "max_slew") {
            float val;
            if (iss >> val) maxSlew_ = val;
        } else if (key == "pdaf_gain") {
            float val;
            if (iss >> val) pdafGain_ = val;
        } else if (key == "pdaf_squelch") {
            float val;
            if (iss >> val) pdafSquelch_ = val;
        } else if (key == "pdaf_conf_thresh") {
            float val;
            if (iss >> val) pdafConfThresh_ = val;
        } else if (key == "contrast_ratio") {
            float val;
            if (iss >> val) contrastRatio_ = val;
        } else if (key == "conf_epsilon") {
            float val;
            if (iss >> val) confEpsilon_ = val;
        } else if (key == "skip_frames") {
            uint32_t val;
            if (iss >> val) skipFrames_ = val;
        }
    }

    file.close();
    LOG(LibipaAf, Info) << "Config loaded: " << path;
    return 0;
}

int AfAlgo::saveConfig(const std::string& path) const
{
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG(LibipaAf, Error) << "Failed to create config: " << path;
        return -EIO;
    }

    file << "# AF Algorithm Configuration\n";
    file << "[af]\n";
    file << "focus_min " << focusMin_ << "\n";
    file << "focus_max " << focusMax_ << "\n";
    file << "focus_default " << focusDefault_ << "\n";
    file << "step_coarse " << stepCoarse_ << "\n";
    file << "step_fine " << stepFine_ << "\n";
    file << "max_slew " << maxSlew_ << "\n";
    file << "pdaf_gain " << pdafGain_ << "\n";
    file << "pdaf_squelch " << pdafSquelch_ << "\n";
    file << "pdaf_conf_thresh " << pdafConfThresh_ << "\n";
    file << "contrast_ratio " << contrastRatio_ << "\n";
    file << "conf_epsilon " << confEpsilon_ << "\n";
    file << "skip_frames " << skipFrames_ << "\n";

    file.close();
    LOG(LibipaAf, Info) << "Config saved: " << path;
    return 0;
}

void AfAlgo::startScan()
{
    state_ = AfState::Scanning;
    scanDirection_ = 1.0f;
    scanStep_ = stepCoarse_;
    lastContrast_ = 0.0f;
    retriggerCount_ = 0;
    ftarget_ = focusMin_; // Start from near
}

void AfAlgo::goIdle()
{
    state_ = AfState::Idle;
    retriggerCount_ = 0;
    LOG(LibipaAf, Debug) << "AF Go Idle";
}

void AfAlgo::setLensPosition(float dioptres)
{
    ftarget_ = std::clamp(dioptres, focusMin_, focusMax_);
    updateLensPosition();
    mode_ = AfMode::Manual;
    state_ = AfState::Idle;
}

void AfAlgo::triggerScan()
{
    if (mode_ == AfMode::Auto) {
        startScan();
    }
}

void AfAlgo::cancelScan()
{
    if (mode_ == AfMode::Auto && state_ == AfState::Scanning) {
        goIdle();
    }
}

void AfAlgo::updateLensPosition()
{
    // Map dioptres to hardware units (0-1023)
    // Linear mapping: 0 dioptres (infinity) -> 0, maxDioptres -> 1023
    float range = focusMax_ - focusMin_;
    float norm = 0.0f;
    if (range > 0.001f) {
        norm = (ftarget_ - focusMin_) / range;
    }
    
    hwpos_ = static_cast<int32_t>(norm * 1023.0f);
    hwpos_ = std::clamp(hwpos_, 0, 1023);

    // Smooth the transition (simple first-order filter)
    fsmooth_ = fsmooth_ + (ftarget_ - fsmooth_) * 0.2f;

    positionUpdated_ = true;
    LOG(LibipaAf, Debug) << "Lens pos: Target=" << ftarget_ << "D -> " 
                         << hwpos_ << " (smoothed=" << fsmooth_ << "D)";
}

void AfAlgo::doScan(float contrast)
{
    if (retriggerCount_ > 0) {
        retriggerCount_--;
        return;
    }

    if (contrast > lastContrast_) {
        // Moving towards peak
        lastContrast_ = contrast;
        // Keep going in same direction
    } else {
        // Peak passed or noise
        if (lastContrast_ * contrastRatio_ > contrast) {
            // Significant drop - peak found!
            goIdle();
            LOG(LibipaAf, Info) << "AF Scan complete. Peak at " << ftarget_ << "D";
            return;
        } else {
            // Noise or small drop - reverse and reduce step
            scanDirection_ = -scanDirection_;
            scanStep_ = std::max(stepFine_, scanStep_ * 0.5f);
            lastContrast_ = contrast;
        }
    }

    // Move lens
    float delta = scanDirection_ * scanStep_;
    ftarget_ += delta;
    ftarget_ = std::clamp(ftarget_, focusMin_, focusMax_);

    // Check bounds
    if (ftarget_ == focusMin_ || ftarget_ == focusMax_) {
        // Hit limit, reverse direction
        scanDirection_ = -scanDirection_;
        scanStep_ = stepCoarse_;
        
        // If we hit both limits without finding peak, fail
        if (state_ == AfState::Scanning && retriggerCount_ > retriggerDelay_) {
            state_ = AfState::Failed;
            LOG(LibipaAf, Warning) << "AF Scan failed - no peak found";
        }
    }
}

void AfAlgo::doContinuous(float contrast, float phase, float conf)
{
    // PDAF logic (if confidence is high enough)
    if (conf > pdafConfThresh_) {
        float phaseDelta = phase * pdafGain_;
        
        // Scale by confidence
        phaseDelta *= (conf / (conf + confEpsilon_));
        
        // Squelch small movements
        if (std::abs(phaseDelta) < pdafSquelch_) {
            float a = phaseDelta / pdafSquelch_;
            phaseDelta *= a * a;
        }
        
        ftarget_ += phaseDelta;
        ftarget_ = std::clamp(ftarget_, focusMin_, focusMax_);
    } else {
        // Fallback to contrast hill climbing
        if (contrast > lastContrast_) {
            ftarget_ += scanDirection_ * stepFine_;
        } else {
            scanDirection_ = -scanDirection_;
            ftarget_ += scanDirection_ * stepFine_;
        }
        lastContrast_ = contrast;
    }

    // Clamp to range
    ftarget_ = std::clamp(ftarget_, focusMin_, focusMax_);
}

bool AfAlgo::process(float contrast, float phase, float conf)
{
    positionUpdated_ = false;

    // Skip frames for stability
    frameCount_++;
    if (frameCount_ < skipFrames_) {
        return false;
    }
    frameCount_ = 0;

    if (mode_ == AfMode::Manual) {
        return false; // No automatic updates in manual mode
    }

    if (mode_ == AfMode::Auto) {
        if (state_ == AfState::Scanning) {
            doScan(contrast);
            updateLensPosition();
        }
    } else if (mode_ == AfMode::Continuous) {
        doContinuous(contrast, phase, conf);
        updateLensPosition();
    }

    return positionUpdated_;
}

void AfAlgo::handleControls(const ControlList &controls)
{
    bool modeChanged = false;
    bool rangeChanged = false;
    bool speedChanged = false;

    // 1. Handle AF Mode (Standard Control)
    if (controls.contains(controls::AfMode)) {
        auto val = controls.get(controls::AfMode);
        if (val.type() == ControlTypeInteger) {
            int32_t modeVal = val.toInt32();
            AfMode newMode;
            switch (modeVal) {
            case 0: newMode = AfMode::Manual; break;
            case 1: newMode = AfMode::Auto; break;
            case 2: newMode = AfMode::Continuous; break;
            default: newMode = AfMode::Manual; break;
            }
            if (mode_ != newMode) {
                setMode(newMode);
                modeChanged = true;
            }
        }
    }

    // 2. Handle Focus Range (Custom Controls or Standard if available)
    // Using custom control IDs for now (assuming softisp.mojom defines them)
    // If using standard controls, replace with controls::LensPositionMin/Max
    if (controls.contains(controls::CustomAfFocusMin)) {
        auto val = controls.get(controls::CustomAfFocusMin);
        if (val.type() == ControlTypeFloat) {
            float minVal = val.toFloat();
            if (std::abs(minVal - focusMin_) > 0.01f) {
                setRange(minVal, focusMax_, focusDefault_);
                rangeChanged = true;
            }
        }
    }

    if (controls.contains(controls::CustomAfFocusMax)) {
        auto val = controls.get(controls::CustomAfFocusMax);
        if (val.type() == ControlTypeFloat) {
            float maxVal = val.toFloat();
            if (std::abs(maxVal - focusMax_) > 0.01f) {
                setRange(focusMin_, maxVal, focusDefault_);
                rangeChanged = true;
            }
        }
    }

    // 3. Handle Speed Parameters (Custom Controls)
    if (controls.contains(controls::CustomAfStepCoarse)) {
        auto val = controls.get(controls::CustomAfStepCoarse);
        if (val.type() == ControlTypeFloat) {
            float valF = val.toFloat();
            if (std::abs(valF - stepCoarse_) > 0.01f) {
                setSpeed(valF, stepFine_, maxSlew_);
                speedChanged = true;
            }
        }
    }

    if (controls.contains(controls::CustomAfStepFine)) {
        auto val = controls.get(controls::CustomAfStepFine);
        if (val.type() == ControlTypeFloat) {
            float valF = val.toFloat();
            if (std::abs(valF - stepFine_) > 0.01f) {
                setSpeed(stepCoarse_, valF, maxSlew_);
                speedChanged = true;
            }
        }
    }

    // 4. Handle Manual Lens Position
    if (controls.contains(controls::LensPosition)) {
        auto val = controls.get(controls::LensPosition);
        if (val.type() == ControlTypeFloat) {
            float dioptres = val.toFloat();
            setLensPosition(dioptres);
            LOG(LibipaAf, Debug) << "Manual lens position set: " << dioptres << "D";
        }
    }

    if (modeChanged) {
        LOG(LibipaAf, Info) << "AF Mode changed via ControlList";
    }
    if (rangeChanged) {
        LOG(LibipaAf, Info) << "AF Range changed via ControlList";
    }
    if (speedChanged) {
        LOG(LibipaAf, Debug) << "AF Speed changed via ControlList";
    }
}

} // namespace libipa
} // namespace libcamera
