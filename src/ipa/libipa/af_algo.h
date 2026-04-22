/* SPDX-License-Identifier: LGPL-2.1-or-later */
/**
 * libipa::AfAlgo - Hardware-agnostic Auto-Focus Algorithm
 * 
 * Adapted from Raspberry Pi controller AF algorithm.
 * Supports PDAF (Phase Detection) and CDAF (Contrast Detection) modes.
 * 
 * This class implements the "Control Loop" (Brain) of the AF system.
 * It takes focus metrics as input and outputs lens positions.
 */
#pragma once

#include <libcamera/base/log.h>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <string>

namespace libcamera {
namespace libipa {

/**
 * AF Algorithm Modes
 */
enum class AfMode {
    Manual = 0,      ///< Direct lens control via setLensPosition()
    Auto = 1,        ///< One-shot scan to find peak contrast
    Continuous = 2   ///< Continuous tracking (PDAF + CDAF hybrid)
};

/**
 * AF Algorithm States
 */
enum class AfState {
    Idle = 0,        ///< Not focusing
    Scanning = 1,    ///< Scanning for peak (Auto mode)
    Focusing = 2,    ///< Actively tracking (Continuous mode)
    Failed = 3       ///< Focus failed (out of range)
};

/**
 * AfAlgo - Auto-Focus Control Loop
 * 
 * This class implements the decision-making logic for autofocus.
 * It receives focus metrics (contrast, phase) and calculates the
 * optimal lens position.
 * 
 * Usage:
 *   AfAlgo af;
 *   af.setRange(0.0f, 12.0f, 1.0f);  // Dioptres
 *   af.setMode(AfMode::Auto);
 *   
 *   // Every frame:
 *   bool updated = af.process(contrastScore, phaseError, confidence);
 *   if (updated) {
 *       int32_t vcm = af.getLensPosition();
 *       // Send vcm to hardware
 *   }
 */
class AfAlgo {
public:
    AfAlgo();
    ~AfAlgo();

    // ============================================================
    // Configuration
    // ============================================================

    /**
     * Set focus range in dioptres (1/meters)
     * @param minDioptres Minimum focus distance (e.g., 0.0 for infinity)
     * @param maxDioptres Maximum focus distance (e.g., 12.0 for 8cm)
     * @param defaultDioptres Default starting position
     */
    void setRange(float minDioptres, float maxDioptres, float defaultDioptres);

    /**
     * Set movement speed parameters
     * @param stepCoarse Large step size (for fast scanning)
     * @param stepFine Small step size (for precision)
     * @param maxSlew Maximum movement speed (dioptres/frame)
     */
    void setSpeed(float stepCoarse, float stepFine, float maxSlew);

    /**
     * Set AF mode
     * @param mode Manual, Auto, or Continuous
     */
    void setMode(AfMode mode);

    /**
     * Set PDAF-specific parameters
     * @param gain Loop gain for PDAF correction (typically negative)
     * @param squelch Minimum movement threshold to suppress wobble
     * @param confThresh Minimum confidence to trust PDAF data
     */
    void setPdafParams(float gain, float squelch, float confThresh);

    /**
     * Set contrast detection parameters
     * @param ratio Ratio to detect peak (0.75 = 75% of peak)
     * @param epsilon Smoothing factor for confidence
     * @param skipFrames Frames to skip between updates (stability)
     */
    void setCdafParams(float ratio, float epsilon, uint32_t skipFrames);

    /**
     * Load configuration from a simple key-value file
     * Format:
     *   [af]
     *   focus_min = 0.0
     *   focus_max = 12.0
     *   step_coarse = 1.0
     *   ...
     * @param path Path to config file
     * @return 0 on success, negative on error
     */
    int loadConfig(const std::string& path);

    /**
     * Save current configuration to file
     * @param path Path to config file
     * @return 0 on success, negative on error
     */
    int saveConfig(const std::string& path) const;

    // ============================================================
    // Main Processing Loop
    // ============================================================

    /**
     * Process focus metrics and calculate new lens position
     * 
     * This is the main entry point, called once per frame (or every few frames).
     * 
     * @param contrast Current contrast score (0.0 to 1.0, higher = sharper)
     * @param phase Phase detection error in pixels (positive = lens too close)
     * @param conf Confidence in phase data (0.0 to 1.0)
     * @return true if a new lens position is ready to be sent to hardware
     */
    bool process(float contrast, float phase = 0.0f, float conf = 0.0f);

    // ============================================================
    // Manual Control
    // ============================================================

    /**
     * Set manual lens position (overrides automatic focus)
     * @param dioptres Target focus in dioptres
     */
    void setLensPosition(float dioptres);

    /**
     * Trigger a new auto-focus scan (Auto mode only)
     */
    void triggerScan();

    /**
     * Cancel current scan and return to idle
     */
    void cancelScan();

    // ============================================================
    // Getters
    // ============================================================

    /**
     * Get current lens position in hardware units (VCM steps, 0-1023)
     */
    int32_t getLensPosition() const { return hwpos_; }

    /**
     * Get current target position in dioptres
     */
    float getTargetDioptres() const { return ftarget_; }

    /**
     * Get current smoothed position in dioptres
     */
    float getSmoothedDioptres() const { return fsmooth_; }

    /**
     * Get current AF state
     */
    AfState getState() const { return state_; }

    /**
     * Get current AF mode
     */
    AfMode getMode() const { return mode_; }

    /**
     * Check if position was updated in last process() call
     */
    bool hasNewPosition() const { return positionUpdated_; }

    /**
     * Get default lens position
     */
    float getDefaultLensPosition() const { return focusDefault_; }

    /**
     * Get minimum lens position
     */
    float getMinLensPosition() const { return focusMin_; }

    /**
     * Get maximum lens position
     */
    float getMaxLensPosition() const { return focusMax_; }

private:
    // State variables
    AfMode mode_;
    AfState state_;
    float ftarget_;      // Target focus (dioptres)
    float fsmooth_;      // Smoothed focus (dioptres)
    int32_t hwpos_;      // Hardware position (VCM steps)
    bool positionUpdated_;

    // Range parameters
    float focusMin_;
    float focusMax_;
    float focusDefault_;

    // Speed parameters
    float stepCoarse_;
    float stepFine_;
    float maxSlew_;

    // PDAF parameters
    float pdafGain_;
    float pdafSquelch_;
    float pdafConfThresh_;

    // CDAF parameters
    float contrastRatio_;
    float confEpsilon_;
    uint32_t skipFrames_;
    uint32_t frameCount_;

    // Scan state
    float lastContrast_;
    float scanDirection_;  // +1 or -1
    float scanStep_;
    uint32_t retriggerDelay_;
    uint32_t retriggerCount_;

    // Helper functions
    void updateLensPosition();
    void doScan(float contrast);
    void doContinuous(float contrast, float phase, float conf);
    void goIdle();
    void startScan();
};

} // namespace libipa
} // namespace libcamera
