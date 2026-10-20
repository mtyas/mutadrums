// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>
#include "PlaitsDrums.h"
#include "BraidsDrums.h"

namespace Mutadrums {

// -------------------------------------------------------------------------------------
//  Engines
// -------------------------------------------------------------------------------------
enum DrumEngineType {
    ENGINE_PLAITS_ANALOG_BD = 0,
    ENGINE_PLAITS_SYNTH_BD,
    ENGINE_PLAITS_ANALOG_SD,
    ENGINE_PLAITS_SYNTH_SD,
    ENGINE_PLAITS_HI_HAT,
    ENGINE_PLAITS_HI_HAT_OPEN,
    ENGINE_PLAITS_MODAL,
    ENGINE_PLAITS_PARTICLE,
    ENGINE_BRAIDS_KICK,
    ENGINE_BRAIDS_SNARE,
    ENGINE_BRAIDS_CYMBAL,
    ENGINE_BRAIDS_BELL,
    NUM_ENGINES
};

struct EngineInfo {
    const char* name;       // full name
    const char* shortName;  // pad label
    const char* toneLabel;  // what the TONE knob does on this engine
    const char* charLabel;  // what the CHARACTER knob does on this engine
    const char* blurb;      // one-line description
    float basePitchHz;      // pitch at Tune = 0
};

inline const EngineInfo& getEngineInfo(int type) {
    static const EngineInfo infos[NUM_ENGINES] = {
        { "Analog Kick",  "808 Kick",  "Tone",       "Drive",      "Bridged-T resonator with pitch dive and diode drive (808 style).", 50.0f },
        { "Synth Kick",   "909 Kick",  "Click",      "Drive",      "Swept sine with band-passed click and waveshaper (909 style).",    50.0f },
        { "Analog Snare", "808 Snare", "Noise Tone", "Snappy",     "Twin resonant shells plus filtered noise (808 style).",          180.0f },
        { "Synth Snare",  "909 Snare", "Noise Tone", "Snappy",     "Two swept oscillators plus high-passed noise (909 style).",       180.0f },
        { "Closed Hat",   "Closed Hat","Colour",     "Noise",      "Six square-wave metallic cluster, band-passed. Short decay.",       82.0f },
        { "Open Hat",     "Open Hat",  "Colour",     "Noise",      "Metallic cluster with long decay — put it in a choke group with a closed hat.", 82.0f },
        { "Modal Resonator",        "Modal",     "Brightness", "Inharmonic", "Physical resonator: woodblock -> tom -> bell as Inharmonic rises.", 160.0f },
        { "Clap",         "Clap",      "Body",       "Spread",     "Multi-burst handclap into a resonant band-pass.",                 125.0f },
        { "Pulse Kick",         "Pulse Kick",   "Tone",       "Drive",      "Pulse-excited SVF resonator kick with saturation.",               50.0f },
        { "Resonant Snare",        "Res Snare",  "Noise Tone", "Snappy",     "Dual SVF resonators plus noise burst.",                          180.0f },
        { "Metal Cymbal",       "Cymbal",    "Colour",     "Noise",      "Six detuned oscillators with metallic filtering.",               100.0f },
        { "Additive Bell",         "Bell",      "Brightness", "Inharmonic", "Additive inharmonic partials — bells, gongs, struck metal.",     220.0f },
    };
    return infos[std::clamp(type, 0, NUM_ENGINES - 1)];
}

inline const char* getEngineName(int type) { return getEngineInfo(type).name; }

// -------------------------------------------------------------------------------------
//  Triggers
// -------------------------------------------------------------------------------------
enum TriggerSource {
    TRIG_MIDI_ONLY = 0,
    TRIG_GRIDS_BD,
    TRIG_GRIDS_SD,
    TRIG_GRIDS_HH,
    TRIG_MARBLES_T1,
    TRIG_MARBLES_T2,
    TRIG_MARBLES_T3,
    NUM_TRIG_SOURCES
};

inline const char* getTriggerSourceName(int src) {
    static const char* names[NUM_TRIG_SOURCES] = {
        "MIDI / Pad only", "Topo BD", "Topo SD", "Topo HH", "Chance t1", "Chance t2", "Chance t3"
    };
    return (src >= 0 && src < NUM_TRIG_SOURCES) ? names[src] : "MIDI / Pad only";
}

// -------------------------------------------------------------------------------------
//  Modulation — evaluated ONCE per hit (sample & hold), so each drum hit gets its own
//  variation and nothing jumps in the middle of a decaying sound.
// -------------------------------------------------------------------------------------
enum ModSource {
    MOD_SRC_OFF = 0,
    MOD_SRC_X1,        // Marbles X1 random CV (bipolar)
    MOD_SRC_X2,
    MOD_SRC_X3,
    MOD_SRC_VELOCITY,  // 0 at full velocity, negative for softer hits
    MOD_SRC_RANDOM,    // fresh random value per hit (bipolar)
    MOD_SRC_LFO,       // global tempo-synced LFO (bipolar)
    MOD_SRC_STEP,      // position in the bar 0..1 (ramp)
    NUM_MOD_SOURCES
};

inline const char* getModSourceName(int src) {
    static const char* names[NUM_MOD_SOURCES] = {
        "Off", "Chance X1", "Chance X2", "Chance X3", "Velocity", "Random", "LFO", "Bar Ramp"
    };
    return (src >= 0 && src < NUM_MOD_SOURCES) ? names[src] : "Off";
}

enum ModDestination {
    MOD_DEST_PITCH = 0,
    MOD_DEST_DECAY,
    MOD_DEST_TONE,
    MOD_DEST_CHARACTER,
    MOD_DEST_LEVEL,
    MOD_DEST_PAN,
    NUM_MOD_DESTINATIONS
};

inline const char* getModDestName(int dest) {
    static const char* names[NUM_MOD_DESTINATIONS] = { "Tune", "Decay", "Tone", "Character", "Level", "Pan" };
    return (dest >= 0 && dest < NUM_MOD_DESTINATIONS) ? names[dest] : "Tune";
}

static constexpr int kNumModSlots = 3;

struct ModSlot {
    int source{ MOD_SRC_OFF };
    int dest{ MOD_DEST_PITCH };
    float amount{ 0.0f }; // -1..+1
};

// Values of the global mod sources at the moment of a hit
struct ModContext {
    float x[3]{ 0.5f, 0.5f, 0.5f }; // Marbles X1..X3, 0..1
    float lfo{ 0.0f };               // -1..1
    float barPos{ 0.0f };            // 0..1
};

// -------------------------------------------------------------------------------------
//  Voice
// -------------------------------------------------------------------------------------
class VoiceEngine {
public:
    VoiceEngine() { reset(); }

    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        plaitsAnalogBD_.init(sampleRate);
        plaitsSynthBD_.init(sampleRate);
        plaitsAnalogSD_.init(sampleRate);
        plaitsSynthSD_.init(sampleRate);
        plaitsHiHat_.init(sampleRate);
        plaitsModal_.init(sampleRate);
        plaitsParticle_.init(sampleRate);
        braidsKick_.init(sampleRate);
        braidsSnare_.init(sampleRate);
        braidsCymbal_.init(sampleRate);
        braidsBell_.init(sampleRate);
        chokeStep_ = 1.0f / (0.004f * sampleRate); // 4 ms declick on choke
        dirty_ = true;
    }

    void reset() {
        plaitsAnalogBD_.reset();
        plaitsSynthBD_.reset();
        plaitsAnalogSD_.reset();
        plaitsSynthSD_.reset();
        plaitsHiHat_.reset();
        plaitsModal_.reset();
        plaitsParticle_.reset();
        braidsKick_.reset();
        braidsSnare_.reset();
        braidsCymbal_.reset();
        braidsBell_.reset();
        chokeGain_ = 1.0f;
        choking_ = false;
        modOffsets_.fill(0.0f);
        dirty_ = true;
    }

    void setSeed(uint32_t seed) { rng_ = seed * 2654435761u + 12345u; }

    // Fire the voice. Modulation is sampled here.
    void trigger(float velocity, const ModContext& ctx) {
        velocity = std::clamp(velocity, 0.05f, 1.0f);

        // Sample & hold all modulation for this hit
        std::array<float, NUM_MOD_DESTINATIONS> offs{};
        const float rnd = randomBipolar();
        for (const auto& slot : slots_) {
            if (slot.source == MOD_SRC_OFF || std::abs(slot.amount) < 1.0e-4f) continue;
            if (slot.dest < 0 || slot.dest >= NUM_MOD_DESTINATIONS) continue;
            float v = 0.0f;
            switch (slot.source) {
                case MOD_SRC_X1: v = (ctx.x[0] - 0.5f) * 2.0f; break;
                case MOD_SRC_X2: v = (ctx.x[1] - 0.5f) * 2.0f; break;
                case MOD_SRC_X3: v = (ctx.x[2] - 0.5f) * 2.0f; break;
                case MOD_SRC_VELOCITY: v = velocity - 1.0f; break;
                case MOD_SRC_RANDOM: v = rnd; break;
                case MOD_SRC_LFO: v = ctx.lfo; break;
                case MOD_SRC_STEP: v = ctx.barPos; break;
                default: break;
            }
            offs[static_cast<size_t>(slot.dest)] += v * slot.amount;
        }
        modOffsets_ = offs;
        dirty_ = true;

        chokeGain_ = 1.0f;
        choking_ = false;
        lastVelocity_ = velocity;
        triggerEngine(velocity);
    }

    // Gracefully silence (used by choke groups): short fade then hard stop.
    void choke() {
        if (!choking_) choking_ = true;
    }

    inline float process() {
        if (dirty_) updateEffective();

        float s = renderEngine();
        if (choking_) {
            chokeGain_ -= chokeStep_;
            if (chokeGain_ <= 0.0f) {
                chokeEngine();
                chokeGain_ = 1.0f;
                choking_ = false;
                return 0.0f;
            }
            s *= chokeGain_;
        }
        return s * levelGain_;
    }

    // ---- Base parameters (set once per block from the host parameters)
    void setEngineType(int type) {
        type = std::clamp(type, 0, NUM_ENGINES - 1);
        if (type != engineType_) {
            reset();
            engineType_ = type;
            dirty_ = true;
        }
    }
    int getEngineType() const { return engineType_; }

    void setTune(float semis)   { if (semis != tune_)  { tune_ = semis; dirty_ = true; } }
    void setDecay(float v)      { v = std::clamp(v, 0.0f, 1.0f); if (v != decay_) { decay_ = v; dirty_ = true; } }
    void setTone(float v)       { v = std::clamp(v, 0.0f, 1.0f); if (v != tone_)  { tone_ = v;  dirty_ = true; } }
    void setCharacter(float v)  { v = std::clamp(v, 0.0f, 1.0f); if (v != char_)  { char_ = v;  dirty_ = true; } }

    void setModSlot(int idx, int source, int dest, float amount) {
        if (idx < 0 || idx >= kNumModSlots) return;
        slots_[static_cast<size_t>(idx)] = { source, dest, std::clamp(amount, -1.0f, 1.0f) };
    }

    // Latched per-hit modulation, for the GUI and the channel strip (pan)
    float getModOffset(int dest) const { return modOffsets_[static_cast<size_t>(std::clamp(dest, 0, NUM_MOD_DESTINATIONS - 1))]; }
    float getPanOffset() const { return std::clamp(modOffsets_[MOD_DEST_PAN], -1.0f, 1.0f); }

private:
    float randomBipolar() {
        rng_ = rng_ * 1664525u + 1013904223u;
        return static_cast<float>(rng_ >> 8) / 8388608.0f - 1.0f;
    }

    void updateEffective() {
        dirty_ = false;
        const float semis = std::clamp(tune_ + modOffsets_[MOD_DEST_PITCH] * 24.0f, -48.0f, 48.0f);
        pitchHz_ = getEngineInfo(engineType_).basePitchHz * std::pow(2.0f, semis / 12.0f);
        effDecay_ = std::clamp(decay_ + modOffsets_[MOD_DEST_DECAY] * 0.5f, 0.0f, 1.0f);
        effTone_ = std::clamp(tone_ + modOffsets_[MOD_DEST_TONE] * 0.5f, 0.0f, 1.0f);
        effChar_ = std::clamp(char_ + modOffsets_[MOD_DEST_CHARACTER] * 0.5f, 0.0f, 1.0f);
        const float levelDb = std::clamp(modOffsets_[MOD_DEST_LEVEL] * 24.0f, -60.0f, 12.0f);
        levelGain_ = std::pow(10.0f, levelDb / 20.0f);
    }

    void triggerEngine(float velocity) {
        switch (engineType_) {
            case ENGINE_PLAITS_ANALOG_BD:   plaitsAnalogBD_.trigger(velocity); break;
            case ENGINE_PLAITS_SYNTH_BD:    plaitsSynthBD_.trigger(velocity); break;
            case ENGINE_PLAITS_ANALOG_SD:   plaitsAnalogSD_.trigger(velocity); break;
            case ENGINE_PLAITS_SYNTH_SD:    plaitsSynthSD_.trigger(velocity); break;
            case ENGINE_PLAITS_HI_HAT:      plaitsHiHat_.trigger(velocity, false); break;
            case ENGINE_PLAITS_HI_HAT_OPEN: plaitsHiHat_.trigger(velocity, true); break;
            case ENGINE_PLAITS_MODAL:       plaitsModal_.trigger(velocity); break;
            case ENGINE_PLAITS_PARTICLE:    plaitsParticle_.trigger(velocity); break;
            case ENGINE_BRAIDS_KICK:        braidsKick_.trigger(velocity); break;
            case ENGINE_BRAIDS_SNARE:       braidsSnare_.trigger(velocity); break;
            case ENGINE_BRAIDS_CYMBAL:      braidsCymbal_.trigger(velocity); break;
            case ENGINE_BRAIDS_BELL:        braidsBell_.trigger(velocity); break;
            default: break;
        }
    }

    void chokeEngine() {
        switch (engineType_) {
            case ENGINE_PLAITS_ANALOG_BD:   plaitsAnalogBD_.choke(); break;
            case ENGINE_PLAITS_SYNTH_BD:    plaitsSynthBD_.choke(); break;
            case ENGINE_PLAITS_ANALOG_SD:   plaitsAnalogSD_.choke(); break;
            case ENGINE_PLAITS_SYNTH_SD:    plaitsSynthSD_.choke(); break;
            case ENGINE_PLAITS_HI_HAT:
            case ENGINE_PLAITS_HI_HAT_OPEN: plaitsHiHat_.choke(); break;
            case ENGINE_PLAITS_MODAL:       plaitsModal_.choke(); break;
            case ENGINE_PLAITS_PARTICLE:    plaitsParticle_.choke(); break;
            case ENGINE_BRAIDS_KICK:        braidsKick_.choke(); break;
            case ENGINE_BRAIDS_SNARE:       braidsSnare_.choke(); break;
            case ENGINE_BRAIDS_CYMBAL:      braidsCymbal_.choke(); break;
            case ENGINE_BRAIDS_BELL:        braidsBell_.choke(); break;
            default: break;
        }
    }

    inline float renderEngine() {
        const float p = pitchHz_, d = effDecay_, t = effTone_, c = effChar_;
        switch (engineType_) {
            case ENGINE_PLAITS_ANALOG_BD:   return plaitsAnalogBD_.process(p, d, t, c);
            case ENGINE_PLAITS_SYNTH_BD:    return plaitsSynthBD_.process(p, d, t, c);
            case ENGINE_PLAITS_ANALOG_SD:   return plaitsAnalogSD_.process(p, d, t, c);
            case ENGINE_PLAITS_SYNTH_SD:    return plaitsSynthSD_.process(p, d, t, c);
            case ENGINE_PLAITS_HI_HAT:
            case ENGINE_PLAITS_HI_HAT_OPEN: return plaitsHiHat_.process(p, d, t, c);
            case ENGINE_PLAITS_MODAL:       return plaitsModal_.process(p, d, t, c);
            case ENGINE_PLAITS_PARTICLE:    return plaitsParticle_.process(p, d, t, c);
            case ENGINE_BRAIDS_KICK:        return braidsKick_.process(p, d, t, c);
            case ENGINE_BRAIDS_SNARE:       return braidsSnare_.process(p, d, t, c);
            case ENGINE_BRAIDS_CYMBAL:      return braidsCymbal_.process(p, d, t, c);
            case ENGINE_BRAIDS_BELL:        return braidsBell_.process(p, d, t, c);
            default:                        return 0.0f;
        }
    }

    float sampleRate_{ 44100.0f };
    int engineType_{ ENGINE_PLAITS_ANALOG_BD };

    float tune_{ 0.0f }, decay_{ 0.5f }, tone_{ 0.5f }, char_{ 0.5f };
    float pitchHz_{ 50.0f }, effDecay_{ 0.5f }, effTone_{ 0.5f }, effChar_{ 0.5f }, levelGain_{ 1.0f };
    bool dirty_{ true };

    std::array<ModSlot, kNumModSlots> slots_{};
    std::array<float, NUM_MOD_DESTINATIONS> modOffsets_{};
    float lastVelocity_{ 1.0f };
    uint32_t rng_{ 0x1234567u };

    float chokeGain_{ 1.0f };
    float chokeStep_{ 0.005f };
    bool choking_{ false };

    PlaitsAnalogBD plaitsAnalogBD_;
    PlaitsSyntheticBD plaitsSynthBD_;
    PlaitsAnalogSD plaitsAnalogSD_;
    PlaitsSyntheticSD plaitsSynthSD_;
    PlaitsHiHat plaitsHiHat_;
    PlaitsModalVoice plaitsModal_;
    PlaitsParticle plaitsParticle_;
    BraidsKick braidsKick_;
    BraidsSnare braidsSnare_;
    BraidsCymbal braidsCymbal_;
    BraidsBell braidsBell_;
};

} // namespace Mutadrums
