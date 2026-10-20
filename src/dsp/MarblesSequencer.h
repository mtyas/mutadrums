// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.
//
// Based on Mutable Instruments Marbles by Emilie Gillet (MIT licence).

#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <random>
#include <vector>

namespace Mutadrums {

class MarblesSequencer {
public:
    enum TModel {
        MODEL_BERNOULLI = 0,   // Coin toss branching between t1 and t3
        MODEL_POLYRHYTHM = 1,  // Multipliers/dividers & cluster polyrhythms
        MODEL_DRUMS = 2        // Generative syncopated drum rhythms
    };

    MarblesSequencer() {
        reset();
    }

    void reset() {
        stepCounter_ = 0;
        running_ = false;
        tRate_ = 1.0f;       // 1x (normal 16th or 8th clock)
        tJitter_ = 0.0f;     // Timing jitter / humanize
        tBias_ = 0.5f;       // Balance between t1 and t3
        dejaVuAmount_ = 0.0f;// 0 = pure random, 1 = locked loop
        dejaVuLength_ = 16;  // Loop length in steps
        tModel_ = MODEL_BERNOULLI;

        historyIndex_ = 0;
        for (int i = 0; i < kMaxHistory; ++i) {
            historyDecisions_[i] = 0;
            historyVoltages_[i][0] = 0.5f;
            historyVoltages_[i][1] = 0.5f;
            historyVoltages_[i][2] = 0.5f;
        }

        gates_[0] = gates_[1] = gates_[2] = false;
        voltages_[0] = voltages_[1] = voltages_[2] = 0.5f;
    }

    void setRate(float rate) { tRate_ = std::clamp(rate, 0.25f, 4.0f); }
    float getRate() const { return tRate_; }
    void setJitter(float jitter) { tJitter_ = std::clamp(jitter, 0.0f, 1.0f); }
    void setBias(float bias) { tBias_ = std::clamp(bias, 0.0f, 1.0f); }
    void setDejaVu(float amount) { dejaVuAmount_ = std::clamp(amount, 0.0f, 1.0f); }
    void setLoopLength(int length) { dejaVuLength_ = std::clamp(length, 1, 16); }
    void setModel(TModel model) { tModel_ = model; }
    void setRunning(bool running) { running_ = running; }

    bool isRunning() const { return running_; }
    int getLoopLength() const { return dejaVuLength_; }
    float getDejaVu() const { return dejaVuAmount_; }
    float getBias() const { return tBias_; }
    float getJitter() const { return tJitter_; }
    TModel getModel() const { return tModel_; }

    // Clock step triggered by transport or internal tick
    void stepClock(int currentStepIndex = -1) {
        if (currentStepIndex >= 0) {
            historyIndex_ = currentStepIndex % std::max(1, dejaVuLength_);
            stepCounter_ = currentStepIndex;
        } else {
            historyIndex_ = (historyIndex_ + 1) % dejaVuLength_;
        }

        // Determine if we recall from Déjà-Vu history or generate new random decision
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float recallProb = dist(rng_);
        bool useHistory = (recallProb < dejaVuAmount_);

        uint8_t decision = 0;
        if (useHistory) {
            decision = historyDecisions_[historyIndex_];
            for (int i = 0; i < 3; ++i) {
                voltages_[i] = historyVoltages_[historyIndex_][i];
            }
        } else {
            // Generate new decision based on model
            decision = generateDecision();
            historyDecisions_[historyIndex_] = decision;

            // Generate stepped random CVs for X generator
            for (int i = 0; i < 3; ++i) {
                // Bias skews the distribution: 0.5 = uniform, >0.5 favours high values
                const float shape = std::pow(4.0f, (0.5f - tBias_) * 2.0f);
                float v = std::pow(dist(rng_), shape);
                voltages_[i] = v;
                historyVoltages_[historyIndex_][i] = v;
            }
        }

        // Apply decision to gates: bit 0 = t1, bit 1 = t2, bit 2 = t3
        gates_[0] = (decision & 1) != 0;
        gates_[1] = (decision & 2) != 0;
        gates_[2] = (decision & 4) != 0;

        stepCounter_++;
    }

    bool getGate(int channel) const {
        if (channel >= 0 && channel < 3) return gates_[channel];
        return false;
    }

    float getVoltage(int channel) const {
        if (channel >= 0 && channel < 3) return voltages_[channel];
        return 0.5f;
    }

    // Jitter offset in samples or fractions of a step
    float getTimingJitter() {
        if (tJitter_ <= 0.001f) return 0.0f;
        std::normal_distribution<float> normDist(0.0f, tJitter_ * 0.15f);
        return normDist(rng_);
    }

    int getHistoryIndex() const { return historyIndex_; }

private:
    static const int kMaxHistory = 16;

    uint8_t generateDecision() {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        uint8_t d = 0;

        switch (tModel_) {
            case MODEL_BERNOULLI: {
                // t2 always fires on beat
                d |= 2;
                // Coin toss with bias: bias < 0.5 favors t1, bias > 0.5 favors t3
                float coin = dist(rng_);
                if (coin > tBias_) {
                    d |= 1; // t1
                } else {
                    d |= 4; // t3
                }
                break;
            }

            case MODEL_POLYRHYTHM: {
                // Polyrhythmic subdivisions
                // t2 fires on quarter note / 4 steps
                if ((stepCounter_ % 4) == 0) d |= 2;
                // t1 fires every 3 steps (3 against 4)
                if ((stepCounter_ % 3) == 0) d |= 1;
                // t3 fires with probability based on bias
                if (dist(rng_) < tBias_) d |= 4;
                break;
            }

            case MODEL_DRUMS: {
                // Generative syncopated drum rhythms:
                // Kick on t1, Snare on t2, Hats on t3
                int stepInBar = stepCounter_ % 16;
                // Kick anchor on 0 and 8, plus syncopated hits based on bias
                if (stepInBar == 0 || (stepInBar == 8 && dist(rng_) < 0.8f)) {
                    d |= 1;
                } else if ((stepInBar == 6 || stepInBar == 10 || stepInBar == 14) && dist(rng_) < (1.0f - tBias_)) {
                    d |= 1;
                }

                // Snare anchor on 4 and 12, plus ghost notes
                if (stepInBar == 4 || stepInBar == 12) {
                    d |= 2;
                } else if ((stepInBar == 7 || stepInBar == 15) && dist(rng_) < (tBias_ * 0.6f)) {
                    d |= 2;
                }

                // Hats on 8ths or 16ths depending on bias
                if ((stepInBar % 2) == 0 || dist(rng_) < tBias_) {
                    d |= 4;
                }
                break;
            }
        }
        return d;
    }

    float tRate_{ 1.0f };
    float tJitter_{ 0.0f };
    float tBias_{ 0.5f };
    float dejaVuAmount_{ 0.0f };
    int dejaVuLength_{ 16 };
    TModel tModel_{ MODEL_BERNOULLI };
    bool running_{ false };

    int stepCounter_{ 0 };
    int historyIndex_{ 0 };
    uint8_t historyDecisions_[kMaxHistory]{ 0 };
    float historyVoltages_[kMaxHistory][3]{ {0.5f, 0.5f, 0.5f} };

    bool gates_[3]{ false, false, false };
    float voltages_[3]{ 0.5f, 0.5f, 0.5f };

    std::mt19937 rng_{ 4242 };
};

} // namespace Mutadrums
