// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace Mutadrums {

// =====================================================================================
//  FxReverb — 8-line Feedback Delay Network (FDN) reverb
//
//  Why an FDN: the feedback matrix is orthogonal (Householder) and every line has a
//  gain < 1 derived directly from the requested RT60, so the loop is unconditionally
//  stable — it cannot run away into feedback no matter how the controls are set.
//
//  Signal flow:
//    mono in -> high-pass (kills low-end mud) -> pre-delay -> 4 series allpass diffusers
//    -> 8 modulated delay lines (one-pole damping per line) -> Householder mix -> back in
//    -> decorrelated stereo taps -> width (M/S) -> return gain
// =====================================================================================
class FxReverb {
public:
    static constexpr int kNumLines = 8;
    static constexpr float kOutGain = 3.5f;

    void init(float sampleRate) {
        sampleRate_ = sampleRate;

        preDelay_.assign(static_cast<size_t>(0.25f * sampleRate) + 4, 0.0f);
        preWrite_ = 0;

        // Input diffuser lengths (ms) — mutually prime-ish, short
        static const float apMs[4] = { 4.771f, 3.595f, 12.73f, 9.307f };
        for (int i = 0; i < 4; ++i) {
            diffusers_[i].init(std::max(1, static_cast<int>(apMs[i] * 0.001f * sampleRate)));
        }

        // FDN line lengths (ms). Spread between ~30 and ~80 ms for a dense, room/plate-like tail.
        static const float lineMs[kNumLines] = { 31.71f, 37.11f, 41.13f, 43.73f, 53.31f, 59.93f, 67.73f, 79.17f };
        const int modHeadroom = static_cast<int>(0.004f * sampleRate) + 8;
        for (int i = 0; i < kNumLines; ++i) {
            lineLen_[i] = lineMs[i] * 0.001f * sampleRate;
            lines_[i].assign(static_cast<size_t>(lineLen_[i]) + modHeadroom, 0.0f);
            lineWrite_[i] = 0;
            lfoPhase_[i] = static_cast<float>(i) / kNumLines;
            lfoInc_[i] = (0.11f + 0.07f * static_cast<float>(i)) / sampleRate; // 0.11 .. 0.6 Hz
        }
        modDepth_ = 0.0011f * sampleRate; // ~1.1 ms excursion — enough to smear metallic modes

        dirty_ = true;
        updateCoefficients();
        reset();
    }

    void reset() {
        std::fill(preDelay_.begin(), preDelay_.end(), 0.0f);
        for (auto& d : diffusers_) d.reset();
        for (int i = 0; i < kNumLines; ++i) {
            std::fill(lines_[i].begin(), lines_[i].end(), 0.0f);
            damp_[i] = 0.0f;
        }
        hpState_ = 0.0f;
        returnSmoothed_ = returnTarget_;
    }

    // decay01:   0..1  -> RT60 0.3 s .. 12 s (exponential)
    // bright01:  0..1  -> loop damping lowpass 1.5 kHz .. 18 kHz
    // preDelayMs 0..200
    // width01:   0 = mono, 1 = full stereo
    // returnDB:  -60 (off) .. +6
    void setParameters(float decay01, float bright01, float preDelayMs, float width01, float returnDB) {
        decay01 = std::clamp(decay01, 0.0f, 1.0f);
        bright01 = std::clamp(bright01, 0.0f, 1.0f);
        if (decay01 != decay01Param_ || bright01 != bright01Param_) {
            decay01Param_ = decay01;
            bright01Param_ = bright01;
            dirty_ = true;
        }
        preDelaySamples_ = std::clamp(static_cast<int>(preDelayMs * 0.001f * sampleRate_), 0,
                                      static_cast<int>(preDelay_.size()) - 2);
        width_ = std::clamp(width01, 0.0f, 1.0f);
        returnTarget_ = returnDB <= -59.0f ? 0.0f : std::pow(10.0f, returnDB / 20.0f);
        updateCoefficients();
    }

    static float decayToSeconds(float decay01) {
        return 0.3f * std::pow(40.0f, std::clamp(decay01, 0.0f, 1.0f));
    }

    float getRT60() const { return decayToSeconds(decay01Param_); }

    inline void process(float inL, float inR, float& outL, float& outR) {
        returnSmoothed_ += (returnTarget_ - returnSmoothed_) * 0.002f;

        if (preDelay_.empty()) { outL = outR = 0.0f; return; }

        // 1. Input high-pass (one-pole, ~120 Hz)
        float x = 0.5f * (inL + inR);
        hpState_ += hpCoeff_ * (x - hpState_);
        x -= hpState_;

        // 2. Pre-delay (correct: delay 0 == current sample)
        preDelay_[preWrite_] = x;
        int rd = preWrite_ - preDelaySamples_;
        if (rd < 0) rd += static_cast<int>(preDelay_.size());
        float pd = preDelay_[static_cast<size_t>(rd)];
        if (++preWrite_ >= static_cast<int>(preDelay_.size())) preWrite_ = 0;

        // 3. Input diffusion
        pd = diffusers_[0].process(pd, 0.70f);
        pd = diffusers_[1].process(pd, 0.70f);
        pd = diffusers_[2].process(pd, 0.60f);
        pd = diffusers_[3].process(pd, 0.60f);

        // 4. Read all lines (modulated, linear interpolation), damp, apply decay gain
        float y[kNumLines];
        float sum = 0.0f;
        for (int i = 0; i < kNumLines; ++i) {
            lfoPhase_[i] += lfoInc_[i];
            if (lfoPhase_[i] >= 1.0f) lfoPhase_[i] -= 1.0f;
            float tri = 4.0f * std::abs(lfoPhase_[i] - 0.5f) - 1.0f; // -1..1 triangle
            float d = lineLen_[i] + tri * modDepth_;

            float v = readLine(i, d);
            damp_[i] += dampCoeff_ * (v - damp_[i]);
            y[i] = damp_[i] * lineGain_[i];
            sum += y[i];
        }

        // 5. Householder feedback matrix: A = I - (2/N) * 1*1^T  (orthogonal, energy-preserving)
        const float h = sum * (2.0f / kNumLines);
        static const float inSign[kNumLines] = { 1, -1, 1, -1, 1, -1, 1, -1 };
        for (int i = 0; i < kNumLines; ++i) {
            float fb = y[i] - h;
            writeLine(i, fb + pd * inSign[i] * 0.35f);
        }

        // 6. Stereo output taps (decorrelated sign patterns)
        // Output gain calibrated so a drum hit at 0 dB send / 0 dB return gives a tail of
        // roughly the same perceived loudness as the dry hit at the default decay.
        float wetL = ( y[0] + y[1] - y[2] + y[3] - y[4] + y[5] + y[6] - y[7]) * kOutGain;
        float wetR = ( y[0] - y[1] + y[2] + y[3] + y[4] - y[5] + y[6] + y[7]) * kOutGain;

        // 7. Width
        float mid = 0.5f * (wetL + wetR);
        float side = 0.5f * (wetL - wetR) * width_;
        outL = (mid + side) * returnSmoothed_;
        outR = (mid - side) * returnSmoothed_;
    }

private:
    class Allpass {
    public:
        void init(int len) { buf_.assign(static_cast<size_t>(len), 0.0f); pos_ = 0; }
        void reset() { std::fill(buf_.begin(), buf_.end(), 0.0f); pos_ = 0; }
        // Schroeder allpass: w[n] = x + g*w[n-M]; y = -g*w[n] + w[n-M]
        inline float process(float x, float g) {
            float delayed = buf_[pos_];
            float w = x + g * delayed;
            buf_[pos_] = w;
            if (++pos_ >= buf_.size()) pos_ = 0;
            return delayed - g * w;
        }
    private:
        std::vector<float> buf_;
        size_t pos_{ 0 };
    };

    inline float readLine(int i, float delay) const {
        const auto& b = lines_[i];
        const int n = static_cast<int>(b.size());
        float rp = static_cast<float>(lineWrite_[i]) - delay;
        while (rp < 0.0f) rp += static_cast<float>(n);
        int i0 = static_cast<int>(rp);
        float frac = rp - static_cast<float>(i0);
        if (i0 >= n) i0 -= n;
        int i1 = i0 + 1;
        if (i1 >= n) i1 = 0;
        return b[static_cast<size_t>(i0)] + frac * (b[static_cast<size_t>(i1)] - b[static_cast<size_t>(i0)]);
    }

    inline void writeLine(int i, float v) {
        auto& b = lines_[i];
        b[static_cast<size_t>(lineWrite_[i])] = v;
        if (++lineWrite_[i] >= static_cast<int>(b.size())) lineWrite_[i] = 0;
    }

    void updateCoefficients() {
        if (!dirty_) return;
        dirty_ = false;
        const float rt60 = decayToSeconds(decay01Param_);
        for (int i = 0; i < kNumLines; ++i) {
            // Gain so that the signal falls 60 dB in rt60 seconds through this line
            lineGain_[i] = std::pow(10.0f, -3.0f * lineLen_[i] / (rt60 * sampleRate_));
        }
        const float dampHz = 1500.0f * std::pow(12.0f, bright01Param_);
        dampCoeff_ = 1.0f - std::exp(-6.2831853f * std::min(dampHz, 0.45f * sampleRate_) / sampleRate_);
        hpCoeff_ = 1.0f - std::exp(-6.2831853f * 120.0f / sampleRate_);
    }

    float sampleRate_{ 44100.0f };

    std::vector<float> preDelay_;
    int preWrite_{ 0 };
    int preDelaySamples_{ 0 };

    std::array<Allpass, 4> diffusers_;

    std::array<std::vector<float>, kNumLines> lines_;
    std::array<int, kNumLines> lineWrite_{};
    std::array<float, kNumLines> lineLen_{};
    std::array<float, kNumLines> lineGain_{};
    std::array<float, kNumLines> damp_{};
    std::array<float, kNumLines> lfoPhase_{};
    std::array<float, kNumLines> lfoInc_{};
    float modDepth_{ 40.0f };

    float decay01Param_{ 0.6f };
    float bright01Param_{ 0.5f };
    bool dirty_{ true };
    float dampCoeff_{ 0.5f };
    float hpCoeff_{ 0.02f };
    float hpState_{ 0.0f };

    float width_{ 0.85f };
    float returnTarget_{ 0.3f };
    float returnSmoothed_{ 0.3f };
};

} // namespace Mutadrums
