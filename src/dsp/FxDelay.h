// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

namespace Mutadrums {

// =====================================================================================
//  FxDelay — stereo / ping-pong tape-flavoured delay
//
//  * True ping-pong: the (mono) send enters the LEFT line; the feedback crosses over to
//    the other side, so echoes bounce L -> R -> L. "Ping-Pong" blends from a plain
//    dual-mono echo (0) to a full bounce (1).
//  * Tone filtering and soft saturation live INSIDE the feedback loop, so every repeat
//    gets darker/warmer like a real tape or BBD delay, and the loop is gain-bounded.
//  * Time / division changes crossfade between two read heads (no pitch-warble chirp).
//  * "Tape" adds gentle saturation and a subtle, slow wow — 0 is a clean digital delay.
// =====================================================================================
class FxDelay {
public:
    enum DelayDivision {
        DIV_1_32 = 0, DIV_1_16_T, DIV_1_16, DIV_1_16_D, DIV_1_8_T, DIV_1_8,
        DIV_1_8_D, DIV_1_4_T, DIV_1_4, DIV_1_4_D, DIV_1_2, NUM_DIVISIONS
    };

    static inline const char* getDivisionName(int div) {
        static const char* names[NUM_DIVISIONS] = {
            "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2"
        };
        return (div >= 0 && div < NUM_DIVISIONS) ? names[div] : "1/8";
    }

    static inline float getDivisionBeats(int div) {
        static const float beats[NUM_DIVISIONS] = {
            0.125f, 1.0f / 6.0f, 0.25f, 0.375f, 1.0f / 3.0f, 0.5f, 0.75f, 2.0f / 3.0f, 1.0f, 1.5f, 2.0f
        };
        return (div >= 0 && div < NUM_DIVISIONS) ? beats[div] : 0.5f;
    }

    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        const size_t maxSamples = static_cast<size_t>(sampleRate * 4.2f) + 16;
        bufL_.assign(maxSamples, 0.0f);
        bufR_.assign(maxSamples, 0.0f);
        computeTarget();
        curDelay_ = nextDelay_ = targetDelay_;
        xfade_ = 1.0f;
        xfadeInc_ = 1.0f / (0.08f * sampleRate);
        updateFilters();
        reset();
    }

    void reset() {
        std::fill(bufL_.begin(), bufL_.end(), 0.0f);
        std::fill(bufR_.begin(), bufR_.end(), 0.0f);
        writePos_ = 0;
        lpL1_ = lpL2_ = lpR1_ = lpR2_ = 0.0f;
        hpL_ = hpR_ = 0.0f;
        wowPhase_ = 0.0f;
        curDelay_ = nextDelay_ = targetDelay_;
        xfade_ = 1.0f;
        returnSmoothed_ = returnTarget_;
    }

    void setBpm(float bpm) {
        bpm = std::clamp(bpm, 30.0f, 300.0f);
        if (bpm != bpm_) { bpm_ = bpm; computeTarget(); }
    }

    bool getSync() const { return syncToBpm_; }
    int getDivision() const { return division_; }
    float getEffectiveTimeMs() const { return targetDelay_ * 1000.0f / sampleRate_; }

    void setParameters(float timeMs, float feedback, float tone01, float pingPong01, float returnDB,
                       bool sync, int division, float tape01) {
        timeMs_ = std::clamp(timeMs, 5.0f, 2000.0f);
        feedback_ = std::clamp(feedback, 0.0f, 0.95f);
        pingPong_ = std::clamp(pingPong01, 0.0f, 1.0f);
        returnTarget_ = returnDB <= -59.0f ? 0.0f : std::pow(10.0f, returnDB / 20.0f);
        syncToBpm_ = sync;
        division_ = std::clamp(division, 0, NUM_DIVISIONS - 1);
        tape_ = std::clamp(tape01, 0.0f, 1.0f);
        if (tone01 != tone_) { tone_ = std::clamp(tone01, 0.0f, 1.0f); updateFilters(); }
        computeTarget();
    }

    inline void process(float inL, float inR, float& outL, float& outR) {
        if (bufL_.empty()) { outL = outR = 0.0f; return; }

        returnSmoothed_ += (returnTarget_ - returnSmoothed_) * 0.002f;

        // Start a crossfade to a new delay time when the target moves
        if (xfade_ >= 1.0f && std::abs(targetDelay_ - curDelay_) > 1.0f) {
            nextDelay_ = targetDelay_;
            xfade_ = 0.0f;
        }

        // Subtle wow (tape only): ~0.35 Hz, up to ~0.35 ms
        wowPhase_ += 0.35f / sampleRate_;
        if (wowPhase_ >= 1.0f) wowPhase_ -= 1.0f;
        const float wow = std::sin(wowPhase_ * 6.2831853f) * tape_ * 0.00035f * sampleRate_;

        float tapL, tapR;
        if (xfade_ < 1.0f) {
            const float a = 1.0f - xfade_;
            tapL = a * read(bufL_, curDelay_ + wow) + xfade_ * read(bufL_, nextDelay_ + wow);
            tapR = a * read(bufR_, curDelay_ + wow) + xfade_ * read(bufR_, nextDelay_ + wow);
            xfade_ += xfadeInc_;
            if (xfade_ >= 1.0f) { xfade_ = 1.0f; curDelay_ = nextDelay_; }
        } else {
            tapL = read(bufL_, curDelay_ + wow);
            tapR = read(bufR_, curDelay_ + wow);
        }

        // Loop filtering: 12 dB/oct lowpass (two one-poles) + one-pole highpass (~90 Hz)
        lpL1_ += lpCoeff_ * (tapL - lpL1_); lpL2_ += lpCoeff_ * (lpL1_ - lpL2_);
        lpR1_ += lpCoeff_ * (tapR - lpR1_); lpR2_ += lpCoeff_ * (lpR1_ - lpR2_);
        hpL_ += hpCoeff_ * (lpL2_ - hpL_);
        hpR_ += hpCoeff_ * (lpR2_ - hpR_);
        float eL = lpL2_ - hpL_;
        float eR = lpR2_ - hpR_;

        // Soft saturation (unity gain for small signals, bounded for large)
        const float drv = 1.0f + tape_ * 2.0f;
        eL = std::tanh(eL * drv) / drv;
        eR = std::tanh(eR * drv) / drv;

        // Feedback with ping-pong cross-over
        const float w = pingPong_;
        const float fbL = feedback_ * ((1.0f - w) * eL + w * eR);
        const float fbR = feedback_ * ((1.0f - w) * eR + w * eL);

        const float mono = 0.5f * (inL + inR);
        bufL_[writePos_] = mono + fbL;
        bufR_[writePos_] = mono * (1.0f - w) + fbR;
        if (++writePos_ >= bufL_.size()) writePos_ = 0;

        outL = eL * returnSmoothed_ * 1.6f; // ~+4 dB makeup for the loop filtering
        outR = eR * returnSmoothed_ * 1.6f;
    }

private:
    void computeTarget() {
        float ms = syncToBpm_ ? (60000.0f / bpm_) * getDivisionBeats(division_) : timeMs_;
        ms = std::clamp(ms, 5.0f, 4000.0f);
        targetDelay_ = std::clamp(ms * 0.001f * sampleRate_, 4.0f,
                                  bufL_.empty() ? 4.0f : static_cast<float>(bufL_.size() - 8));
    }

    void updateFilters() {
        const float lpHz = std::min(1500.0f * std::pow(12.0f, tone_), 0.45f * sampleRate_); // 1.5k .. 18k
        lpCoeff_ = 1.0f - std::exp(-6.2831853f * lpHz / sampleRate_);
        hpCoeff_ = 1.0f - std::exp(-6.2831853f * 90.0f / sampleRate_);
    }

    // 4-point Hermite read. delay (samples) >= 1: 1 == most recently written sample.
    inline float read(const std::vector<float>& b, float delay) const {
        const int n = static_cast<int>(b.size());
        float rp = static_cast<float>(writePos_) - delay;
        while (rp < 0.0f) rp += static_cast<float>(n);
        int i1 = static_cast<int>(rp);
        const float f = rp - static_cast<float>(i1);
        if (i1 >= n) i1 -= n;
        int i0 = i1 - 1; if (i0 < 0) i0 += n;
        int i2 = i1 + 1; if (i2 >= n) i2 -= n;
        int i3 = i2 + 1; if (i3 >= n) i3 -= n;
        const float ym1 = b[static_cast<size_t>(i0)], y0 = b[static_cast<size_t>(i1)];
        const float y1 = b[static_cast<size_t>(i2)], y2 = b[static_cast<size_t>(i3)];
        const float c1 = 0.5f * (y1 - ym1);
        const float c2 = ym1 - 2.5f * y0 + 2.0f * y1 - 0.5f * y2;
        const float c3 = 0.5f * (y2 - ym1) + 1.5f * (y0 - y1);
        return ((c3 * f + c2) * f + c1) * f + y0;
    }

    float sampleRate_{ 44100.0f };
    float bpm_{ 120.0f };
    bool syncToBpm_{ true };
    int division_{ DIV_1_8_D };
    float timeMs_{ 375.0f };
    float feedback_{ 0.4f };
    float pingPong_{ 0.7f };
    float tone_{ 0.5f };
    float tape_{ 0.5f };
    float returnTarget_{ 0.3f };
    float returnSmoothed_{ 0.3f };

    float targetDelay_{ 16000.0f };
    float curDelay_{ 16000.0f };
    float nextDelay_{ 16000.0f };
    float xfade_{ 1.0f };
    float xfadeInc_{ 0.0003f };

    std::vector<float> bufL_, bufR_;
    size_t writePos_{ 0 };

    float lpCoeff_{ 0.5f }, hpCoeff_{ 0.01f };
    float lpL1_{ 0 }, lpL2_{ 0 }, lpR1_{ 0 }, lpR2_{ 0 }, hpL_{ 0 }, hpR_{ 0 };
    float wowPhase_{ 0.0f };
};

} // namespace Mutadrums
