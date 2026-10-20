// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.
//
// Percussion models based on Mutable Instruments Braids by Emilie Gillet (MIT licence).

#pragma once
#include <cmath>
#include <algorithm>
#include <random>
#include "PlaitsDrums.h"

namespace Mutadrums {

// --- BRAIDS KICK DRUM (Digital SVF Resonator + Pulse Excitation) ---
class BraidsKick {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        pulse1_ = pulse2_ = 0.0f;
        pitchEnv_ = 0.0f;
        ampEnv_ = 0.0f;
        resonator_.reset();
        lpFilter_.reset();
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        pulse1_ = 1.0f * velocity_;
        pulse2_ = -0.6f * velocity_;
        pitchEnv_ = 1.0f;
        ampEnv_ = 1.0f * velocity_;
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float driveParam) {
        if (ampEnv_ < 0.00001f) return 0.0f;

        // Pulses decay
        pulse1_ *= std::exp(-1.0f / (sampleRate_ * 0.001f));
        pulse2_ *= std::exp(-1.0f / (sampleRate_ * 0.003f));
        float excitation = pulse1_ + pulse2_;

        // Pitch dive
        pitchEnv_ *= std::exp(-1.0f / (sampleRate_ * (0.015f + 0.02f * driveParam)));
        float instPitch = pitchHz * (1.0f + 3.0f * pitchEnv_ * pitchEnv_);
        instPitch = std::clamp(instPitch, 20.0f, sampleRate_ * 0.45f);

        // SVF bandpass resonator
        float q = 12.0f + decayParam * 40.0f;
        resonator_.setParameters(instPitch, q, sampleRate_);
        float resOut = resonator_.processBP(excitation * 2.5f);

        // Low-pass tone
        float lpCutoff = std::clamp(pitchHz * (3.0f + toneParam * 12.0f), 80.0f, 16000.0f);
        lpFilter_.setParameters(lpCutoff, 1.0f, sampleRate_);
        float filtered = lpFilter_.processLP(resOut + excitation * 0.2f);

        // Saturated drive
        float drive = 1.2f + driveParam * 3.8f;
        float saturated = std::tanh(filtered * drive);

        // Decay envelope
        float decayTime = 0.06f + decayParam * 1.5f;
        ampEnv_ *= std::exp(-1.0f / (sampleRate_ * decayTime));

        return saturated * ampEnv_;
    }

    void choke() { ampEnv_ = 0.0f; }

private:
    float sampleRate_{ 44100.0f };
    float pulse1_{ 0.0f }, pulse2_{ 0.0f };
    float pitchEnv_{ 0.0f };
    float ampEnv_{ 0.0f };
    float velocity_{ 1.0f };
    CytomicSVF resonator_;
    CytomicSVF lpFilter_;
};

// --- BRAIDS SNARE DRUM (Dual SVF Resonators + Noise Burst) ---
class BraidsSnare {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        pulse1_ = pulse2_ = 0.0f;
        noiseEnv_ = 0.0f;
        ampEnv_ = 0.0f;
        res1_.reset();
        res2_.reset();
        noiseFilter_.reset();
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        pulse1_ = 1.0f * velocity_;
        pulse2_ = 0.7f * velocity_;
        noiseEnv_ = 1.0f * velocity_;
        ampEnv_ = 1.0f * velocity_;
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float snappyParam) {
        if (ampEnv_ < 0.00001f && noiseEnv_ < 0.00001f) return 0.0f;

        pulse1_ *= std::exp(-1.0f / (sampleRate_ * 0.0008f));
        pulse2_ *= std::exp(-1.0f / (sampleRate_ * 0.0018f));

        // Braids dual resonant frequencies: base pitch + octave
        float f1 = std::clamp(pitchHz, 100.0f, 600.0f);
        float f2 = std::clamp(pitchHz * 2.1f, 200.0f, 1400.0f);

        res1_.setParameters(f1, 14.0f + decayParam * 10.0f, sampleRate_);
        res2_.setParameters(f2, 10.0f + decayParam * 8.0f, sampleRate_);

        float body = res1_.processBP(pulse1_) * 0.8f + res2_.processBP(pulse2_) * 0.6f;

        // Snappy noise
        float rawNoise = ((static_cast<float>(rng_() & 0xFFFF) / 32768.0f) - 1.0f);
        noiseFilter_.setParameters(std::clamp(1500.0f + toneParam * 6000.0f, 800.0f, 12000.0f), 1.5f, sampleRate_);
        float filteredNoise = noiseFilter_.processBP(rawNoise);

        float decayTime = 0.05f + decayParam * 0.35f;
        ampEnv_ *= std::exp(-1.0f / (sampleRate_ * decayTime));

        float noiseDecayTime = 0.03f + decayParam * (0.1f + snappyParam * 0.35f);
        noiseEnv_ *= std::exp(-1.0f / (sampleRate_ * noiseDecayTime));

        float out = body * ampEnv_ * (1.0f - snappyParam * 0.4f) + filteredNoise * noiseEnv_ * (0.4f + snappyParam * 1.1f);
        return std::tanh(out * 1.6f);
    }

    void choke() {
        ampEnv_ = 0.0f;
        noiseEnv_ = 0.0f;
    }

private:
    float sampleRate_{ 44100.0f };
    float pulse1_{ 0.0f }, pulse2_{ 0.0f };
    float ampEnv_{ 0.0f }, noiseEnv_{ 0.0f };
    float velocity_{ 1.0f };
    CytomicSVF res1_;
    CytomicSVF res2_;
    CytomicSVF noiseFilter_;
    std::minstd_rand rng_{ 333 };
};

// --- BRAIDS CYMBAL / METALLIC (6 Detuned Oscillators + SVF Bandpass/Highpass) ---
class BraidsCymbal {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        for (int i = 0; i < 6; ++i) phases_[i] = 0.0f;
        ampEnv_ = 0.0f;
        bpFilter_.reset();
        hpFilter_.reset();
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        ampEnv_ = 1.0f * velocity_;
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float morphParam) {
        if (ampEnv_ < 0.00001f) return 0.0f;

        // Braids inharmonic ratios for metallic cymbal
        static const float ratios[6] = { 1.0f, 1.341f, 1.782f, 2.115f, 2.763f, 3.429f };
        float baseFreq = std::clamp(pitchHz * 3.0f, 200.0f, 2500.0f);

        float cluster = 0.0f;
        for (int i = 0; i < 6; ++i) {
            float f = std::clamp(baseFreq * ratios[i], 100.0f, sampleRate_ * 0.48f);
            phases_[i] += f / sampleRate_;
            if (phases_[i] >= 1.0f) phases_[i] -= 1.0f;
            cluster += (phases_[i] > 0.5f ? 0.35f : -0.35f);
        }

        // Noise crossfade
        float rawNoise = ((static_cast<float>(rng_() & 0xFFFF) / 32768.0f) - 1.0f) * 0.4f;
        float mixed = cluster * (1.0f - morphParam * 0.5f) + rawNoise * (0.2f + morphParam * 0.6f);

        float bpCutoff = std::clamp(4000.0f + toneParam * 8000.0f, 2000.0f, 16000.0f);
        bpFilter_.setParameters(bpCutoff, 2.2f, sampleRate_);
        float bp = bpFilter_.processBP(mixed);

        hpFilter_.setParameters(std::clamp(bpCutoff * 0.6f, 1500.0f, 12000.0f), 0.7f, sampleRate_);
        float hp = hpFilter_.processHP(bp);

        float decayTime = 0.08f + decayParam * 1.8f;
        ampEnv_ *= std::exp(-1.0f / (sampleRate_ * decayTime));

        return std::tanh(hp * 2.5f) * ampEnv_;
    }

    void choke() { ampEnv_ = 0.0f; }

private:
    float sampleRate_{ 44100.0f };
    float phases_[6]{ 0.0f };
    float ampEnv_{ 0.0f };
    float velocity_{ 1.0f };
    CytomicSVF bpFilter_;
    CytomicSVF hpFilter_;
    std::minstd_rand rng_{ 444 };
};

// --- BRAIDS BELL / STRUCK DRUM (Inharmonic Additive Partials with Damped Decay) ---
class BraidsBell {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        for (int i = 0; i < 6; ++i) {
            phases_[i] = 0.0f;
            partialAmps_[i] = 0.0f;
        }
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        for (int i = 0; i < 6; ++i) {
            phases_[i] = 0.0f;
            partialAmps_[i] = velocity_ * (1.0f / (1.0f + i * 0.35f));
        }
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float morphParam) {
        float maxAmp = 0.0f;
        for (int i = 0; i < 6; ++i) maxAmp = std::max(maxAmp, partialAmps_[i]);
        if (maxAmp < 0.00001f) return 0.0f;

        // Inharmonic ratios based on morph
        float r[6] = {
            1.0f,
            2.0f + morphParam * 0.76f,
            3.0f + morphParam * 1.83f,
            4.2f + morphParam * 2.91f,
            5.4f + morphParam * 4.25f,
            6.8f + morphParam * 5.72f
        };

        float baseDecay = 0.08f + decayParam * 2.5f;
        float out = 0.0f;

        for (int i = 0; i < 6; ++i) {
            float f = std::clamp(pitchHz * r[i], 30.0f, sampleRate_ * 0.48f);
            phases_[i] += f / sampleRate_;
            if (phases_[i] >= 1.0f) phases_[i] -= 1.0f;

            float sine = std::sin(phases_[i] * 6.2831853f);
            out += sine * partialAmps_[i];

            // Higher partials decay faster
            float decayFactor = baseDecay / (1.0f + i * 1.5f * (1.0f - toneParam * 0.5f));
            partialAmps_[i] *= std::exp(-1.0f / (sampleRate_ * decayFactor));
        }

        return std::tanh(out * 1.5f);
    }

    void choke() {
        for (int i = 0; i < 6; ++i) partialAmps_[i] = 0.0f;
    }

private:
    float sampleRate_{ 44100.0f };
    float phases_[6]{ 0.0f };
    float partialAmps_[6]{ 0.0f };
    float velocity_{ 1.0f };
};

} // namespace Mutadrums
