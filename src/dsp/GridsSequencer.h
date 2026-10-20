// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.
//
// Pattern-generation algorithm adapted from Mutable Instruments Grids,
// Copyright 2012 Emilie Gillet (emilie.o.gillet@gmail.com), GPL-3.0-or-later.

#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <random>
#include "GridsResources.h"

namespace Mutadrums {

class GridsSequencer {
public:
    enum OutputMode {
        MODE_DRUMS = 0,
        MODE_EUCLIDEAN = 1
    };

    GridsSequencer() {
        reset();
    }

    void reset() {
        step_ = 0;
        subStep_ = 0;
        running_ = false;
        state_ = 0;
        accentState_ = 0;
        for (int i = 0; i < 3; ++i) {
            perturbation_[i] = 0;
            euclideanStep_[i] = 0;
            density_[i] = 128;
            euclideanLength_[i] = 16;
            currentLevels_[i] = 0.0f;
        }
        x_ = 128;
        y_ = 128;
        chaos_ = 0;
        swing_ = 0.0f;
        outputMode_ = MODE_DRUMS;
    }

    void setCoordinates(float x, float y) {
        x_ = static_cast<uint8_t>(std::clamp(x * 255.0f, 0.0f, 255.0f));
        y_ = static_cast<uint8_t>(std::clamp(y * 255.0f, 0.0f, 255.0f));
    }

    void setChaos(float chaos) {
        chaos_ = static_cast<uint8_t>(std::clamp(chaos * 255.0f, 0.0f, 255.0f));
    }

    void setDensity(int instrument, float density) {
        if (instrument >= 0 && instrument < 3) {
            density_[instrument] = static_cast<uint8_t>(std::clamp(density * 255.0f, 0.0f, 255.0f));
        }
    }

    void setEuclideanLength(int instrument, int length) {
        if (instrument >= 0 && instrument < 3) {
            euclideanLength_[instrument] = static_cast<uint8_t>(std::clamp(length, 1, 32));
        }
    }

    void setOutputMode(OutputMode mode) {
        outputMode_ = mode;
    }

    void setSwing(float swing) {
        swing_ = std::clamp(swing, 0.0f, 0.75f);
    }

    void setRunning(bool running) {
        running_ = running;
    }

    bool isRunning() const {
        return running_;
    }

    // Advance by 1 step (with optional modulation offsets from Marbles)
    void stepClock(float modOffsetMapX = 0.0f, float modOffsetMapY = 0.0f, float modOffsetChaos = 0.0f, float modOffsetDensity = 0.0f) {
        lastModX_ = modOffsetMapX;
        lastModY_ = modOffsetMapY;
        if (outputMode_ == MODE_DRUMS) {
            evaluateDrums(modOffsetMapX, modOffsetMapY, modOffsetChaos, modOffsetDensity);
        } else {
            evaluateEuclidean(modOffsetDensity);
        }

        // Advance step
        step_ = (step_ + 1) % 32;
        for (int i = 0; i < 3; ++i) {
            euclideanStep_[i] = (euclideanStep_[i] + 1) % euclideanLength_[i];
        }
    }

    // Evaluate the pattern at an absolute 16th-note index (keeps Euclidean lanes bar-aligned)
    void evaluateAt(long long absStep, float modX = 0.0f, float modY = 0.0f, float modChaos = 0.0f, float modDens = 0.0f) {
        if (absStep < 0) absStep = 0;
        step_ = static_cast<uint8_t>(absStep % 32);
        for (int i = 0; i < 3; ++i)
            euclideanStep_[i] = static_cast<uint8_t>(absStep % std::max<int>(1, euclideanLength_[i]));
        lastModX_ = modX;
        lastModY_ = modY;
        if (outputMode_ == MODE_DRUMS) evaluateDrums(modX, modY, modChaos, modDens);
        else evaluateEuclidean(modDens);
    }

    OutputMode getOutputMode() const { return outputMode_; }

    // Set step directly for DAW transport sync
    void setStep(int step, float modOffsetMapX = 0.0f, float modOffsetMapY = 0.0f, float modOffsetChaos = 0.0f, float modOffsetDensity = 0.0f) {
        step_ = step % 32;
        lastModX_ = modOffsetMapX;
        lastModY_ = modOffsetMapY;
        if (outputMode_ == MODE_DRUMS) {
            evaluateDrums(modOffsetMapX, modOffsetMapY, modOffsetChaos, modOffsetDensity);
        } else {
            evaluateEuclidean(modOffsetDensity);
        }
    }

    int getCurrentStep() const { return step_; }
    uint8_t getState() const { return state_; }
    uint8_t getAccentState() const { return accentState_; }

    bool isTriggered(int instrument) const {
        if (instrument < 0 || instrument >= 3) return false;
        return (state_ & (1 << instrument)) != 0;
    }

    bool isAccented(int instrument) const {
        if (instrument < 0 || instrument >= 3) return false;
        return (accentState_ & (1 << instrument)) != 0;
    }

    float getLevel(int instrument) const {
        if (instrument < 0 || instrument >= 3) return 0.0f;
        return currentLevels_[instrument];
    }

    float getX() const { return x_ / 255.0f; }
    float getY() const { return y_ / 255.0f; }
    float getEffectiveX() const { return std::clamp(x_ / 255.0f + lastModX_, 0.0f, 1.0f); }
    float getEffectiveY() const { return std::clamp(y_ / 255.0f + lastModY_, 0.0f, 1.0f); }

private:
    static inline uint8_t u8Mix(uint8_t a, uint8_t b, uint8_t t) {
        return static_cast<uint8_t>((static_cast<uint32_t>(a) * (255 - t) + static_cast<uint32_t>(b) * t) >> 8);
    }

public:
    // Pure lookup into the Grids topographic map (no chaos) — also used by the GUI preview.
    static uint8_t readDrumMap(uint8_t step, uint8_t instrument, uint8_t x, uint8_t y) {
        uint8_t i = x >> 6;
        uint8_t j = y >> 6;
        if (i > 3) i = 3;
        if (j > 3) j = 3;

        const uint8_t* a_map = GridsData::drum_map[i][j];
        const uint8_t* b_map = GridsData::drum_map[i + 1][j];
        const uint8_t* c_map = GridsData::drum_map[i][j + 1];
        const uint8_t* d_map = GridsData::drum_map[i + 1][j + 1];

        uint8_t offset = (instrument * 32) + (step % 32);
        uint8_t a = a_map[offset];
        uint8_t b = b_map[offset];
        uint8_t c = c_map[offset];
        uint8_t d = d_map[offset];

        uint8_t x_frac = (x & 0x3f) << 2;
        uint8_t y_frac = (y & 0x3f) << 2;

        return u8Mix(u8Mix(a, b, x_frac), u8Mix(c, d, x_frac), y_frac);
    }

    // GUI helper: would this instrument fire on this step in Euclidean mode?
    static bool euclideanHit(int length, float density01, int step) {
        length = std::clamp(length, 1, 32);
        const uint8_t density = static_cast<uint8_t>(std::clamp(density01 * 255.0f, 0.0f, 255.0f)) >> 3;
        uint16_t address = static_cast<uint16_t>((length - 1) * 32 + density);
        if (address >= 1024) address = 1023;
        return (GridsData::lut_res_euclidean[address] & (1UL << (step % length))) != 0;
    }

private:
    void evaluateDrums(float modX = 0.0f, float modY = 0.0f, float modChaos = 0.0f, float modDens = 0.0f) {
        state_ = 0;
        accentState_ = 0;

        int effX = std::clamp(static_cast<int>(x_) + static_cast<int>(modX * 255.0f), 0, 255);
        int effY = std::clamp(static_cast<int>(y_) + static_cast<int>(modY * 255.0f), 0, 255);
        int effChaos = std::clamp(static_cast<int>(chaos_) + static_cast<int>(modChaos * 255.0f), 0, 255);

        // At step 0, calculate perturbation if chaos is active
        if (step_ == 0 && effChaos > 0) {
            for (int i = 0; i < 3; ++i) {
                uint8_t rnd = static_cast<uint8_t>(rng_() & 0xFF);
                perturbation_[i] = static_cast<uint8_t>((static_cast<uint32_t>(rnd) * (effChaos >> 2)) >> 8);
            }
        } else if (effChaos == 0) {
            perturbation_[0] = perturbation_[1] = perturbation_[2] = 0;
        }

        for (uint8_t i = 0; i < 3; ++i) {
            uint8_t level = readDrumMap(step_, i, static_cast<uint8_t>(effX), static_cast<uint8_t>(effY));
            if (level < 255 - perturbation_[i]) {
                level += perturbation_[i];
            } else {
                level = 255;
            }

            currentLevels_[i] = level / 255.0f;
            int effDensity = std::clamp(static_cast<int>(density_[i]) + static_cast<int>(modDens * 255.0f), 0, 255);
            uint8_t threshold = static_cast<uint8_t>(255 - effDensity);

            if (level > threshold && effDensity > 0) {
                state_ |= (1 << i);
                if (level > 192) {
                    accentState_ |= (1 << i);
                }
            }
        }
    }

    void evaluateEuclidean(float modDens = 0.0f) {
        state_ = 0;
        accentState_ = 0;

        for (uint8_t i = 0; i < 3; ++i) {
            uint8_t length = euclideanLength_[i];
            if (length < 1) length = 1;
            uint8_t density = density_[i] >> 3;
            uint16_t address = static_cast<uint16_t>((length - 1) * 32 + density);
            if (address >= 1024) address = 1023;

            uint32_t patternBits = GridsData::lut_res_euclidean[address];
            uint32_t stepMask = 1UL << (euclideanStep_[i] % length);
            (void) modDens;

            currentLevels_[i] = density_[i] / 255.0f;

            if (patternBits & stepMask) {
                state_ |= (1 << i);
                if (euclideanStep_[i] == 0) {
                    accentState_ |= (1 << i);
                }
            }
        }
    }

    uint8_t x_{ 128 };
    uint8_t y_{ 128 };
    uint8_t chaos_{ 0 };
    uint8_t density_[3]{ 128, 128, 128 };
    uint8_t euclideanLength_[3]{ 16, 16, 16 };
    uint8_t euclideanStep_[3]{ 0, 0, 0 };
    uint8_t perturbation_[3]{ 0, 0, 0 };
    float currentLevels_[3]{ 0.0f, 0.0f, 0.0f };

    OutputMode outputMode_{ MODE_DRUMS };
    float swing_{ 0.0f };
    bool running_{ false };

    uint8_t step_{ 0 };
    uint8_t subStep_{ 0 };
    uint8_t state_{ 0 };
    uint8_t accentState_{ 0 };
    float lastModX_{ 0.0f };
    float lastModY_{ 0.0f };

    std::minstd_rand rng_{ 1337 };
};

} // namespace Mutadrums
