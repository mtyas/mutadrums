#pragma once
#include <cmath>
#include <algorithm>
#include <complex>

namespace MutableDrums {

// Biquad filter for EQ
class Biquad {
public:
    void reset() {
        x1_ = x2_ = y1_ = y2_ = 0.0f;
    }

    void setLowShelf(float freqHz, float gainDB, float sampleRate) {
        freqHz = std::clamp(freqHz, 20.0f, sampleRate * 0.45f);
        float A = std::pow(10.0f, gainDB / 40.0f);
        float w0 = 6.28318530717958647692f * freqHz / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / 2.0f * std::sqrt(2.0f); // Q = 0.707
        float sqrtA = std::sqrt(A);

        float a0 = (A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
        b0_ = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha)) / a0;
        b1_ = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0)) / a0;
        b2_ = (A * ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha)) / a0;
        a1_ = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0)) / a0;
        a2_ = ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha) / a0;
    }

    void setPeaking(float freqHz, float q, float gainDB, float sampleRate) {
        freqHz = std::clamp(freqHz, 20.0f, sampleRate * 0.45f);
        q = std::clamp(q, 0.1f, 10.0f);
        float A = std::pow(10.0f, gainDB / 40.0f);
        float w0 = 6.28318530717958647692f * freqHz / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * q);

        float a0 = 1.0f + alpha / A;
        b0_ = (1.0f + alpha * A) / a0;
        b1_ = (-2.0f * cosw0) / a0;
        b2_ = (1.0f - alpha * A) / a0;
        a1_ = (-2.0f * cosw0) / a0;
        a2_ = (1.0f - alpha / A) / a0;
    }

    void setHighShelf(float freqHz, float gainDB, float sampleRate) {
        freqHz = std::clamp(freqHz, 20.0f, sampleRate * 0.45f);
        float A = std::pow(10.0f, gainDB / 40.0f);
        float w0 = 6.28318530717958647692f * freqHz / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / 2.0f * std::sqrt(2.0f);
        float sqrtA = std::sqrt(A);

        float a0 = (A + 1.0f) - (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha;
        b0_ = (A * ((A + 1.0f) + (A - 1.0f) * cosw0 + 2.0f * sqrtA * alpha)) / a0;
        b1_ = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0)) / a0;
        b2_ = (A * ((A + 1.0f) + (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha)) / a0;
        a1_ = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0)) / a0;
        a2_ = ((A + 1.0f) - (A - 1.0f) * cosw0 - 2.0f * sqrtA * alpha) / a0;
    }

    void setLowpass(float freqHz, float q, float sampleRate) {
        freqHz = std::clamp(freqHz, 20.0f, sampleRate * 0.45f);
        float w0 = 6.28318530717958647692f * freqHz / sampleRate;
        float cosw0 = std::cos(w0);
        float alpha = std::sin(w0) / (2.0f * q);
        float a0 = 1.0f + alpha;
        b0_ = ((1.0f - cosw0) * 0.5f) / a0;
        b1_ = (1.0f - cosw0) / a0;
        b2_ = b0_;
        a1_ = (-2.0f * cosw0) / a0;
        a2_ = (1.0f - alpha) / a0;
    }

    // Magnitude response (linear) at a frequency — used by the GUI EQ curve
    float magnitudeAt(float freqHz, float sampleRate) const {
        const double w = 2.0 * 3.14159265358979323846 * freqHz / sampleRate;
        const std::complex<double> z1 = std::polar(1.0, -w), z2 = std::polar(1.0, -2.0 * w);
        const auto num = static_cast<double>(b0_) + static_cast<double>(b1_) * z1 + static_cast<double>(b2_) * z2;
        const auto den = 1.0 + static_cast<double>(a1_) * z1 + static_cast<double>(a2_) * z2;
        return static_cast<float>(std::abs(num / den));
    }

    inline float process(float in) {
        float out = b0_ * in + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
        x2_ = x1_;
        x1_ = in;
        y2_ = y1_;
        y1_ = out;
        return out;
    }

private:
    float b0_{ 1.0f }, b1_{ 0.0f }, b2_{ 0.0f }, a1_{ 0.0f }, a2_{ 0.0f };
    float x1_{ 0.0f }, x2_{ 0.0f }, y1_{ 0.0f }, y2_{ 0.0f };
};

// Studio Drum Compressor
class DrumCompressor {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        envelope_ = 0.0f;
        gainReduction_ = 0.0f;
    }

    void reset() {
        envelope_ = 0.0f;
        gainReduction_ = 0.0f;
    }

    void setParameters(float thresholdDB, float ratio, float attackMs, float releaseMs, float makeupDB) {
        thresholdDB_ = thresholdDB;
        ratio_ = std::max(1.0f, ratio);
        attackMs_ = std::max(0.1f, attackMs);
        releaseMs_ = std::max(1.0f, releaseMs);
        makeupDB_ = makeupDB;
        makeupGain_ = std::pow(10.0f, makeupDB / 20.0f);

        attackAlpha_ = std::exp(-1.0f / (sampleRate_ * std::max(0.0001f, attackMs_ * 0.001f)));
        releaseAlpha_ = std::exp(-1.0f / (sampleRate_ * std::max(0.001f, releaseMs_ * 0.001f)));
    }

    inline float process(float in) {
        float absIn = std::abs(in);
        // Envelope follower
        if (absIn > envelope_) {
            envelope_ = attackAlpha_ * envelope_ + (1.0f - attackAlpha_) * absIn;
        } else {
            envelope_ = releaseAlpha_ * envelope_ + (1.0f - releaseAlpha_) * absIn;
        }

        float envDB = 20.0f * std::log10(std::max(0.000001f, envelope_));
        float gainReductionDB = 0.0f;

        if (envDB > thresholdDB_) {
            gainReductionDB = (envDB - thresholdDB_) * (1.0f - 1.0f / ratio_);
        }

        gainReduction_ = gainReductionDB;
        float grLinear = std::pow(10.0f, -gainReductionDB / 20.0f);
        return in * grLinear * makeupGain_;
    }

    float getGainReductionDB() const { return gainReduction_; }
    float getThresholdDB() const { return thresholdDB_; }
    float getRatio() const { return ratio_; }
    float getAttackMs() const { return attackMs_; }
    float getReleaseMs() const { return releaseMs_; }
    float getMakeupDB() const { return makeupDB_; }

private:
    float sampleRate_{ 44100.0f };
    float thresholdDB_{ -12.0f };
    float ratio_{ 4.0f };
    float attackMs_{ 5.0f };
    float releaseMs_{ 60.0f };
    float makeupDB_{ 0.0f };
    float makeupGain_{ 1.0f };
    float attackAlpha_{ 0.9f };
    float releaseAlpha_{ 0.999f };
    float envelope_{ 0.0f };
    float gainReduction_{ 0.0f };
};

// Full Channel Strip: EQ -> Drive -> Compressor -> (sends, fader, pan)
class ChannelStrip {
public:
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        compressor_.init(sampleRate);
        compressor_.setParameters(compThresh_, compRatio_, compAttackMs_, compReleaseMs_, compMakeupDB_);
        smoothCoeff_ = 1.0f - std::exp(-1.0f / (0.012f * sampleRate)); // ~12 ms parameter smoothing
        meterDecay_ = std::exp(-1.0f / (0.25f * sampleRate));
        updateEQ();
        updateDriveTone();
        reset();
    }

    void reset() {
        eqLow_.reset();
        eqMid_.reset();
        eqHigh_.reset();
        driveLP_.reset();
        driveShelf_.reset();
        compressor_.reset();
        peakLevel_ = 0.0f;
        gainL_ = targetGainL_;
        gainR_ = targetGainR_;
        send1_ = send1Target_;
        send2_ = send2Target_;
    }

    // ---- EQ
    void setEQ(float lowF, float lowG, float midF, float midQ, float midG, float highF, float highG) {
        if (lowF == eqLowFreq_ && lowG == eqLowGain_ && midF == eqMidFreq_ && midQ == eqMidQ_ &&
            midG == eqMidGain_ && highF == eqHighFreq_ && highG == eqHighGain_)
            return;
        eqLowFreq_ = lowF; eqLowGain_ = lowG;
        eqMidFreq_ = midF; eqMidQ_ = midQ; eqMidGain_ = midG;
        eqHighFreq_ = highF; eqHighGain_ = highG;
        updateEQ();
    }
    void setEQEnabled(bool on) { eqOn_ = on; }

    // ---- Drive
    void setDrive(bool on, float amount, float tone, float trimDB, int mode) {
        driveOn_ = on;
        drive_ = std::clamp(amount, 0.0f, 1.0f);
        driveMode_ = mode;
        if (trimDB != driveTrimDB_) {
            driveTrimDB_ = trimDB;
            driveTrim_ = std::pow(10.0f, trimDB / 20.0f);
        }
        if (tone != driveTone_) {
            driveTone_ = std::clamp(tone, 0.0f, 1.0f);
            updateDriveTone();
        }
    }

    // ---- Compressor
    void setCompressor(bool on, float threshDB, float ratio, float attackMs, float releaseMs, float makeupDB) {
        compOn_ = on;
        if (threshDB == compThresh_ && ratio == compRatio_ && attackMs == compAttackMs_ &&
            releaseMs == compReleaseMs_ && makeupDB == compMakeupDB_)
            return;
        compThresh_ = threshDB; compRatio_ = ratio; compAttackMs_ = attackMs;
        compReleaseMs_ = releaseMs; compMakeupDB_ = makeupDB;
        compressor_.setParameters(threshDB, ratio, attackMs, releaseMs, makeupDB);
    }

    // ---- Fader / pan / sends / mute. 'audible' folds mute + solo logic together.
    void setMix(float volumeDB, float pan, float send1DB, float send2DB, bool audible) {
        const float vol = (!audible || volumeDB <= -59.0f) ? 0.0f : std::pow(10.0f, volumeDB / 20.0f);
        const float p = std::clamp(pan, -1.0f, 1.0f);
        const float angle = (p + 1.0f) * 0.25f * 3.14159265f; // 0..pi/2, equal power
        targetGainL_ = vol * std::cos(angle);
        targetGainR_ = vol * std::sin(angle);
        send1Target_ = (!audible || send1DB <= -59.0f) ? 0.0f : std::pow(10.0f, send1DB / 20.0f);
        send2Target_ = (!audible || send2DB <= -59.0f) ? 0.0f : std::pow(10.0f, send2DB / 20.0f);
        faderGain_ = vol;
    }

    float getPeakLevel() const { return peakLevel_; }
    float getGainReductionDB() const { return compOn_ ? compressor_.getGainReductionDB() : 0.0f; }

    inline void process(float drySample, float& outL, float& outR, float& aux1Out, float& aux2Out) {
        float s = drySample;

        if (eqOn_) {
            s = eqLow_.process(s);
            s = eqMid_.process(s);
            s = eqHigh_.process(s);
        }

        if (driveOn_ && drive_ > 0.001f) {
            const float k = 1.0f + drive_ * 6.0f;
            if (driveMode_ == 1) {        // Tube: asymmetric, even harmonics
                s = std::tanh(s * k + 0.2f * drive_ * s * s) / std::tanh(k);
            } else if (driveMode_ == 2) { // Clip: hard and punchy
                const float g = 1.0f + drive_ * 4.0f;
                s = std::clamp(s * g, -1.0f, 1.0f) / std::sqrt(g);
            } else {                      // Tape: smooth, odd harmonics
                s = std::tanh(s * k) / std::tanh(k);
            }
            s = driveShelf_.process(driveLP_.process(s));
        }
        s *= (driveOn_ ? driveTrim_ : 1.0f);

        if (compOn_) s = compressor_.process(s);

        gainL_ += smoothCoeff_ * (targetGainL_ - gainL_);
        gainR_ += smoothCoeff_ * (targetGainR_ - gainR_);
        send1_ += smoothCoeff_ * (send1Target_ - send1_);
        send2_ += smoothCoeff_ * (send2Target_ - send2_);

        aux1Out = s * send1_;
        aux2Out = s * send2_;
        outL = s * gainL_;
        outR = s * gainR_;

        const float a = std::abs(s) * faderGain_;
        peakLevel_ = a > peakLevel_ ? a : peakLevel_ * meterDecay_;
    }

private:
    void updateEQ() {
        eqLow_.setLowShelf(eqLowFreq_, eqLowGain_, sampleRate_);
        eqMid_.setPeaking(eqMidFreq_, eqMidQ_, eqMidGain_, sampleRate_);
        eqHigh_.setHighShelf(eqHighFreq_, eqHighGain_, sampleRate_);
    }

    // Drive "Tone": below 0.5 progressively darkens (lowpass 20k -> 1.5k),
    // above 0.5 adds presence (high shelf up to +6 dB @ 3 kHz). 0.5 is neutral.
    void updateDriveTone() {
        if (driveTone_ < 0.5f) {
            const float t = driveTone_ / 0.5f;              // 0..1
            const float hz = 1500.0f * std::pow(13.0f, t);  // 1.5k .. 19.5k
            setDriveLP(hz);
            driveShelf_.setHighShelf(3000.0f, 0.0f, sampleRate_);
        } else {
            setDriveLP(0.0f);
            driveShelf_.setHighShelf(3000.0f, (driveTone_ - 0.5f) * 12.0f, sampleRate_);
        }
    }

    // 12 dB/oct lowpass, or flat when hz is out of range
    void setDriveLP(float hz) {
        if (hz <= 0.0f || hz >= 0.45f * sampleRate_) {
            driveLP_.setPeaking(1000.0f, 1.0f, 0.0f, sampleRate_); // flat
            return;
        }
        driveLP_.setLowpass(hz, 0.707f, sampleRate_);
    }

    float sampleRate_{ 44100.0f };
    float smoothCoeff_{ 0.002f };
    float meterDecay_{ 0.9999f };

    float eqLowFreq_{ 100.0f }, eqLowGain_{ 0.0f };
    float eqMidFreq_{ 1200.0f }, eqMidQ_{ 1.0f }, eqMidGain_{ 0.0f };
    float eqHighFreq_{ 8000.0f }, eqHighGain_{ 0.0f };
    bool eqOn_{ true };
    Biquad eqLow_, eqMid_, eqHigh_;

    bool driveOn_{ true };
    float drive_{ 0.0f }, driveTone_{ 0.5f }, driveTrimDB_{ 0.0f }, driveTrim_{ 1.0f };
    int driveMode_{ 0 };
    Biquad driveLP_, driveShelf_;

    bool compOn_{ false };
    float compThresh_{ -12.0f }, compRatio_{ 4.0f }, compAttackMs_{ 5.0f }, compReleaseMs_{ 60.0f }, compMakeupDB_{ 0.0f };
    DrumCompressor compressor_;

    float targetGainL_{ 0.0f }, targetGainR_{ 0.0f }, gainL_{ 0.0f }, gainR_{ 0.0f };
    float send1Target_{ 0.0f }, send2Target_{ 0.0f }, send1_{ 0.0f }, send2_{ 0.0f };
    float faderGain_{ 0.0f };
    float peakLevel_{ 0.0f };
};

} // namespace MutableDrums
