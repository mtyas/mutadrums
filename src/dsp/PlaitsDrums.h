// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.
//
// Drum models based on Mutable Instruments Plaits by Emilie Gillet (MIT licence).

#pragma once
#include <cmath>
#include <algorithm>
#include <random>

namespace Mutadrums {

// Fast trapezoidal State Variable Filter (SVF)
class CytomicSVF {
public:
    void reset() {
        ic1eq = 0.0f;
        ic2eq = 0.0f;
    }

    void setParameters(float cutoffHz, float q, float sampleRate) {
        cutoffHz = std::clamp(cutoffHz, 10.0f, sampleRate * 0.48f);
        q = std::clamp(q, 0.1f, 40.0f);
        float g = std::tan(3.14159265358979323846f * cutoffHz / sampleRate);
        k = 1.0f / q;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }

    inline void process(float v0, float& lp, float& bp, float& hp) {
        float v3 = v0 - ic2eq;
        float v1 = a1 * ic1eq + a2 * v3;
        float v2 = ic2eq + a2 * ic1eq + a3 * v3;
        ic1eq = 2.0f * v1 - ic1eq;
        ic2eq = 2.0f * v2 - ic2eq;

        lp = v2;
        bp = v1;
        hp = v0 - k * v1 - v2;
    }

    inline float processLP(float v0) {
        float lp, bp, hp;
        process(v0, lp, bp, hp);
        return lp;
    }

    inline float processBP(float v0) {
        float lp, bp, hp;
        process(v0, lp, bp, hp);
        return bp;
    }

    inline float processHP(float v0) {
        float lp, bp, hp;
        process(v0, lp, bp, hp);
        return hp;
    }

private:
    float ic1eq{ 0.0f };
    float ic2eq{ 0.0f };
    float a1{ 0.0f }, a2{ 0.0f }, a3{ 0.0f }, k{ 1.0f };
};

// --- PLAITS ANALOG BASS DRUM (808 Style) ---
class PlaitsAnalogBD {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        phase_ = 0.0f;
        ampEnv_ = 0.0f;
        pitchEnv_ = 0.0f;
        clickEnv_ = 0.0f;
        toneFilter_.reset();
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        ampEnv_ = 1.0f * velocity_;
        pitchEnv_ = 1.0f;
        clickEnv_ = 1.0f;
        phase_ = 0.0f;
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float driveParam) {
        if (ampEnv_ < 0.00001f) return 0.0f;

        // Pitch envelope: fast initial drop for punch
        float pitchDecay = std::exp(-1.0f / (sampleRate_ * (0.008f + 0.03f * driveParam)));
        pitchEnv_ *= pitchDecay;

        float instPitch = pitchHz * (1.0f + 3.5f * pitchEnv_ * (0.5f + 0.5f * driveParam));
        instPitch = std::clamp(instPitch, 20.0f, sampleRate_ * 0.45f);

        phase_ += instPitch / sampleRate_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        // Core sine oscillator
        float osc = std::sin(phase_ * 6.28318530717958647692f);

        // Click transient
        float clickDecay = std::exp(-1.0f / (sampleRate_ * 0.002f));
        clickEnv_ *= clickDecay;
        float click = clickEnv_ * (phase_ < 0.5f ? 1.0f : -1.0f);

        // Mix osc + click
        float raw = osc + click * (0.2f + 0.5f * toneParam);

        // Diode / saturating non-linearity
        float drive = 1.0f + driveParam * 3.5f;
        float sat = std::tanh(raw * drive);

        // Tone low-pass filter
        float cutoff = std::clamp(pitchHz * (2.0f + toneParam * 14.0f), 60.0f, 14000.0f);
        toneFilter_.setParameters(cutoff, 1.0f, sampleRate_);
        float out = toneFilter_.processLP(sat);

        // Amp envelope decay
        float decayTime = 0.08f + decayParam * 1.8f;
        float ampDecay = std::exp(-1.0f / (sampleRate_ * decayTime));
        ampEnv_ *= ampDecay;

        return out * ampEnv_;
    }

    void choke() {
        ampEnv_ = 0.0f;
    }

private:
    float sampleRate_{ 44100.0f };
    float phase_{ 0.0f };
    float ampEnv_{ 0.0f };
    float pitchEnv_{ 0.0f };
    float clickEnv_{ 0.0f };
    float velocity_{ 1.0f };
    CytomicSVF toneFilter_;
};

// --- PLAITS SYNTHETIC BASS DRUM (909 Style) ---
class PlaitsSyntheticBD {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        phase_ = 0.0f;
        ampEnv_ = 0.0f;
        pitchEnv_ = 0.0f;
        clickEnv_ = 0.0f;
        clickFilter_.reset();
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        ampEnv_ = 1.0f * velocity_;
        pitchEnv_ = 1.0f;
        clickEnv_ = 1.0f;
        phase_ = 0.0f;
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float driveParam) {
        if (ampEnv_ < 0.00001f) return 0.0f;

        // Dual-stage pitch drop
        float pitchDecay = std::exp(-1.0f / (sampleRate_ * (0.012f + 0.04f * driveParam)));
        pitchEnv_ *= pitchDecay;

        float instPitch = pitchHz * (1.0f + 4.5f * pitchEnv_ * pitchEnv_);
        instPitch = std::clamp(instPitch, 25.0f, sampleRate_ * 0.45f);

        phase_ += instPitch / sampleRate_;
        if (phase_ >= 1.0f) phase_ -= 1.0f;

        float osc = std::sin(phase_ * 6.28318530717958647692f);

        // 909-style narrow bandpass click pulse
        float clickDecay = std::exp(-1.0f / (sampleRate_ * 0.0035f));
        clickEnv_ *= clickDecay;
        clickFilter_.setParameters(std::clamp(pitchHz * 12.0f + 800.0f, 500.0f, 6000.0f), 4.0f, sampleRate_);
        float click = clickFilter_.processBP(clickEnv_ * (phase_ < 0.25f ? 1.0f : 0.0f));

        float mixed = osc * 1.2f + click * (0.4f + toneParam * 0.8f);

        // Aggressive waveshaping overdrive
        float drive = 1.0f + driveParam * 4.0f;
        float saturated = mixed * drive / (1.0f + 0.5f * std::abs(mixed * drive));

        // Amp envelope
        float decayTime = 0.06f + decayParam * 1.4f;
        float ampDecay = std::exp(-1.0f / (sampleRate_ * decayTime));
        ampEnv_ *= ampDecay;

        return saturated * ampEnv_;
    }

    void choke() { ampEnv_ = 0.0f; }

private:
    float sampleRate_{ 44100.0f };
    float phase_{ 0.0f };
    float ampEnv_{ 0.0f };
    float pitchEnv_{ 0.0f };
    float clickEnv_{ 0.0f };
    float velocity_{ 1.0f };
    CytomicSVF clickFilter_;
};

// --- PLAITS ANALOG SNARE DRUM (808 Style) ---
class PlaitsAnalogSD {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        bodyFilter1_.reset();
        bodyFilter2_.reset();
        noiseHP_.reset();
        noiseBP_.reset();
        bodyEnv_ = 0.0f;
        noiseEnv_ = 0.0f;
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        bodyEnv_ = 1.0f * velocity_;
        noiseEnv_ = 1.0f * velocity_;
        pulse_ = 1.0f;
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float snappyParam) {
        if (bodyEnv_ < 0.00001f && noiseEnv_ < 0.00001f) return 0.0f;

        // Pulse excitation into dual resonant bridges (shell body)
        pulse_ *= std::exp(-1.0f / (sampleRate_ * 0.001f));

        float f1 = std::clamp(pitchHz, 80.0f, 600.0f);
        float f2 = std::clamp(pitchHz * 1.83f, 140.0f, 1200.0f);

        bodyFilter1_.setParameters(f1, 16.0f, sampleRate_);
        bodyFilter2_.setParameters(f2, 12.0f, sampleRate_);

        float body = bodyFilter1_.processBP(pulse_) * 1.0f + bodyFilter2_.processBP(pulse_) * 0.6f;

        float bodyDecayTime = 0.05f + decayParam * 0.35f;
        bodyEnv_ *= std::exp(-1.0f / (sampleRate_ * bodyDecayTime));

        // Snappy white noise
        float rawNoise = (static_cast<float>(rng_() & 0xFFFF) / 32768.0f) - 1.0f;

        noiseHP_.setParameters(std::clamp(1000.0f + toneParam * 2500.0f, 800.0f, 8000.0f), 0.7f, sampleRate_);
        noiseBP_.setParameters(std::clamp(2000.0f + toneParam * 4000.0f, 1000.0f, 10000.0f), 1.5f, sampleRate_);

        float filteredNoise = noiseBP_.processBP(noiseHP_.processHP(rawNoise));

        float noiseDecayTime = 0.04f + decayParam * (0.15f + snappyParam * 0.45f);
        noiseEnv_ *= std::exp(-1.0f / (sampleRate_ * noiseDecayTime));

        float snappy = 0.2f + snappyParam * 1.2f;
        float mixed = (body * bodyEnv_) * (1.0f - snappyParam * 0.5f) + (filteredNoise * noiseEnv_) * snappy;

        return std::tanh(mixed * 1.4f);
    }

    void choke() {
        bodyEnv_ = 0.0f;
        noiseEnv_ = 0.0f;
    }

private:
    float sampleRate_{ 44100.0f };
    float bodyEnv_{ 0.0f };
    float noiseEnv_{ 0.0f };
    float pulse_{ 0.0f };
    float velocity_{ 1.0f };
    CytomicSVF bodyFilter1_;
    CytomicSVF bodyFilter2_;
    CytomicSVF noiseHP_;
    CytomicSVF noiseBP_;
    std::minstd_rand rng_{ 777 };
};

// --- PLAITS SYNTHETIC SNARE DRUM (909 Style) ---
class PlaitsSyntheticSD {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        phase1_ = phase2_ = 0.0f;
        toneEnv_ = 0.0f;
        noiseEnv_ = 0.0f;
        pitchEnv_ = 0.0f;
        noiseFilter_.reset();
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        toneEnv_ = 1.0f * velocity_;
        noiseEnv_ = 1.0f * velocity_;
        pitchEnv_ = 1.0f;
        phase1_ = phase2_ = 0.0f;
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float snappyParam) {
        if (toneEnv_ < 0.00001f && noiseEnv_ < 0.00001f) return 0.0f;

        // Dual swept oscillators for shell
        pitchEnv_ *= std::exp(-1.0f / (sampleRate_ * 0.018f));
        float f1 = pitchHz * (1.0f + 1.2f * pitchEnv_);
        float f2 = pitchHz * 1.6f * (1.0f + 0.8f * pitchEnv_);

        phase1_ += std::clamp(f1, 50.0f, sampleRate_ * 0.45f) / sampleRate_;
        if (phase1_ >= 1.0f) phase1_ -= 1.0f;
        phase2_ += std::clamp(f2, 50.0f, sampleRate_ * 0.45f) / sampleRate_;
        if (phase2_ >= 1.0f) phase2_ -= 1.0f;

        float osc = (std::sin(phase1_ * 6.2831853f) + 0.6f * std::sin(phase2_ * 6.2831853f)) * 0.7f;
        toneEnv_ *= std::exp(-1.0f / (sampleRate_ * (0.05f + decayParam * 0.25f)));

        // Noise
        float rawNoise = (static_cast<float>(rng_() & 0xFFFF) / 32768.0f) - 1.0f;
        noiseFilter_.setParameters(std::clamp(1800.0f + toneParam * 5000.0f, 1000.0f, 12000.0f), 1.2f, sampleRate_);
        float noise = noiseFilter_.processHP(rawNoise);
        noiseEnv_ *= std::exp(-1.0f / (sampleRate_ * (0.05f + decayParam * (0.15f + snappyParam * 0.4f))));

        float out = (osc * toneEnv_) * (1.0f - snappyParam * 0.4f) + (noise * noiseEnv_) * (0.4f + snappyParam * 1.0f);
        return std::tanh(out * 1.5f);
    }

    void choke() {
        toneEnv_ = 0.0f;
        noiseEnv_ = 0.0f;
    }

private:
    float sampleRate_{ 44100.0f };
    float phase1_{ 0.0f }, phase2_{ 0.0f };
    float toneEnv_{ 0.0f }, noiseEnv_{ 0.0f }, pitchEnv_{ 0.0f };
    float velocity_{ 1.0f };
    CytomicSVF noiseFilter_;
    std::minstd_rand rng_{ 888 };
};

// --- PLAITS ANALOG HI-HAT (6-Oscillator Schmitt Trigger Metallic Cluster) ---
class PlaitsHiHat {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        for (int i = 0; i < 6; ++i) phases_[i] = 0.0f;
        ampEnv_ = 0.0f;
        bandpass_.reset();
        highpass_.reset();
    }

    void trigger(float velocity = 1.0f, bool openMode = false) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        ampEnv_ = 1.0f * velocity_;
        openMode_ = openMode;
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float morphParam) {
        if (ampEnv_ < 0.00001f) return 0.0f;

        // 6 Schmitt-trigger oscillator frequencies:
        // Roland 808 ratios: ~205, 304, 369, 522, 540, 800 Hz (scaled by pitch)
        static const float ratios[6] = { 1.0f, 1.48f, 1.80f, 2.54f, 2.63f, 3.89f };
        float baseFreq = std::clamp(pitchHz * 2.5f, 100.0f, 1500.0f);

        uint32_t clusterBit = 0;
        for (int i = 0; i < 6; ++i) {
            float f = std::clamp(baseFreq * ratios[i], 100.0f, sampleRate_ * 0.48f);
            phases_[i] += f / sampleRate_;
            if (phases_[i] >= 1.0f) phases_[i] -= 1.0f;
            if (phases_[i] > 0.5f) clusterBit ^= 1;
        }

        float rawSquare = clusterBit ? 0.8f : -0.8f;
        // Add subtle noise blend controlled by morph
        float rawNoise = ((static_cast<float>(rng_() & 0xFFFF) / 32768.0f) - 1.0f) * morphParam * 0.5f;
        float metallic = rawSquare + rawNoise;

        // Bandpass filter centered around 7kHz - 11kHz
        float bpFreq = std::clamp(5000.0f + toneParam * 7000.0f, 4000.0f, 15000.0f);
        bandpass_.setParameters(bpFreq, 3.0f, sampleRate_);
        float bp = bandpass_.processBP(metallic);

        // Highpass filter to remove low rumble
        highpass_.setParameters(std::clamp(bpFreq * 0.7f, 3000.0f, 12000.0f), 0.7f, sampleRate_);
        float filtered = highpass_.processHP(bp);

        // Decay time: open mode has much longer decay
        float decayTime = openMode_ ? (0.15f + decayParam * 1.5f) : (0.02f + decayParam * 0.25f);
        ampEnv_ *= std::exp(-1.0f / (sampleRate_ * decayTime));

        return std::tanh(filtered * 2.0f) * ampEnv_;
    }

    void choke() {
        // Fast decay to silence on choke (closed hat cutting off open hat)
        ampEnv_ = 0.0f;
    }

private:
    float sampleRate_{ 44100.0f };
    float phases_[6]{ 0.0f };
    float ampEnv_{ 0.0f };
    float velocity_{ 1.0f };
    bool openMode_{ false };
    CytomicSVF bandpass_;
    CytomicSVF highpass_;
    std::minstd_rand rng_{ 999 };
};

// --- PLAITS MODAL RESONATOR (Physical Membrane / Bell / Woodblock / Rim) ---
class PlaitsModalVoice {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        for (int i = 0; i < 4; ++i) {
            resonators_[i].reset();
            modeAmps_[i] = 0.0f;
        }
        pulse_ = 0.0f;
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        pulse_ = 1.0f * velocity_;
        for (int i = 0; i < 4; ++i) {
            modeAmps_[i] = 1.0f * velocity_;
        }
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float structureParam) {
        float maxAmp = 0.0f;
        for (int i = 0; i < 4; ++i) maxAmp = std::max(maxAmp, modeAmps_[i]);
        if (maxAmp < 0.00001f) return 0.0f;

        pulse_ *= std::exp(-1.0f / (sampleRate_ * 0.0008f));

        // Inharmonic structure ratios:
        // structure = 0.0 (harmonic / woodblock / drum skin)
        // structure = 1.0 (inharmonic / bell / chime / metal plate)
        float r[4] = {
            1.0f,
            1.0f + 0.5f + structureParam * 1.256f,
            1.0f + 1.0f + structureParam * 3.404f,
            1.0f + 1.8f + structureParam * 6.133f
        };

        float qBase = 15.0f + decayParam * 90.0f;
        float baseDecay = 0.05f + decayParam * 1.5f;

        float out = 0.0f;
        for (int i = 0; i < 4; ++i) {
            float f = std::clamp(pitchHz * r[i], 30.0f, sampleRate_ * 0.48f);
            resonators_[i].setParameters(f, qBase / (1.0f + i * 0.5f * (1.0f - toneParam)), sampleRate_);
            float modeOut = resonators_[i].processBP(pulse_);

            // Higher modes decay faster based on damping
            float decayFactor = baseDecay / (1.0f + i * 1.2f * (1.0f - toneParam * 0.5f));
            modeAmps_[i] *= std::exp(-1.0f / (sampleRate_ * decayFactor));

            out += modeOut * modeAmps_[i] * (1.0f / (1.0f + i * 0.4f));
        }

        return std::tanh(out * 1.8f);
    }

    void choke() {
        for (int i = 0; i < 4; ++i) modeAmps_[i] = 0.0f;
    }

private:
    float sampleRate_{ 44100.0f };
    float pulse_{ 0.0f };
    float velocity_{ 1.0f };
    float modeAmps_[4]{ 0.0f };
    CytomicSVF resonators_[4];
};

// --- PLAITS PARTICLE / HANDCLAP ENGINE ---
class PlaitsParticle {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        reset();
    }

    void reset() {
        sampleIndex_ = 0;
        tailEnv_ = 0.0f;
        filter_.reset();
        active_ = false;
    }

    void trigger(float velocity = 1.0f) {
        velocity_ = std::clamp(velocity, 0.1f, 1.0f);
        sampleIndex_ = 0;
        tailEnv_ = 1.0f * velocity_;
        active_ = true;
    }

    inline float process(float pitchHz, float decayParam, float toneParam, float spreadParam) {
        if (!active_) return 0.0f;

        float outSample = 0.0f;
        float tSeconds = sampleIndex_ / sampleRate_;

        // Handclap burst profile: 3 pre-bursts at ~0ms, 11ms, 22ms, then main body at 33ms
        float burstSpacing = 0.009f + spreadParam * 0.012f;
        float b1 = std::exp(-std::abs(tSeconds - 0.000f) / 0.0015f);
        float b2 = std::exp(-std::abs(tSeconds - burstSpacing) / 0.0015f);
        float b3 = std::exp(-std::abs(tSeconds - 2.0f * burstSpacing) / 0.0015f);
        float b4 = std::exp(-std::max(0.0f, tSeconds - 3.0f * burstSpacing) / (0.04f + decayParam * 0.4f));

        float totalEnv = (b1 * 0.7f + b2 * 0.8f + b3 * 0.9f + b4 * 1.2f) * velocity_;

        float rawNoise = ((static_cast<float>(rng_() & 0xFFFF) / 32768.0f) - 1.0f);
        float burstNoise = rawNoise * totalEnv;

        // Bandpass filter for handclap body
        float bpFreq = std::clamp(pitchHz * 4.0f + toneParam * 2500.0f, 500.0f, 6000.0f);
        filter_.setParameters(bpFreq, 2.5f, sampleRate_);
        outSample = filter_.processBP(burstNoise);

        sampleIndex_++;
        if (tSeconds > 3.0f * burstSpacing + (0.05f + decayParam * 0.5f)) {
            active_ = false;
        }

        return std::tanh(outSample * 2.2f);
    }

    void choke() { active_ = false; }

private:
    float sampleRate_{ 44100.0f };
    uint32_t sampleIndex_{ 0 };
    float tailEnv_{ 0.0f };
    float velocity_{ 1.0f };
    bool active_{ false };
    CytomicSVF filter_;
    std::minstd_rand rng_{ 555 };
};

} // namespace Mutadrums
