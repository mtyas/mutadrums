// Mutadrums - generative drum machine
// Copyright (C) 2026 mtyas
// SPDX-License-Identifier: GPL-3.0-or-later
//
// This program is free software: you can redistribute it and/or modify it under the terms of the
// GNU General Public License as published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version. See the LICENSE file for details.

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/VoiceEngine.h"
#include "dsp/FxDelay.h"
#include "dsp/FxReverb.h"

namespace Mutadrums {
namespace Params {

static constexpr int kNumVoices = 8;
static constexpr int kNumOutputs = 8; // stereo outputs: Main + Out 2..8

inline juce::String outputName(int bus) { return bus == 0 ? juce::String("Main") : "Out " + juce::String(bus + 1); }

// ---- Global IDs ---------------------------------------------------------------------
namespace ID {
    inline const juce::String run          { "run" };
    inline const juce::String hostSync     { "hostSync" };
    inline const juce::String bpm          { "bpm" };
    inline const juce::String master       { "master" };

    inline const juce::String gridsX       { "gridsX" };
    inline const juce::String gridsY       { "gridsY" };
    inline const juce::String gridsChaos   { "gridsChaos" };
    inline const juce::String gridsDensBD  { "gridsDensBD" };
    inline const juce::String gridsDensSD  { "gridsDensSD" };
    inline const juce::String gridsDensHH  { "gridsDensHH" };
    inline const juce::String gridsMode    { "gridsMode" };
    inline const juce::String gridsSwing   { "gridsSwing" };
    inline const juce::String gridsLenBD   { "gridsLenBD" };
    inline const juce::String gridsLenSD   { "gridsLenSD" };
    inline const juce::String gridsLenHH   { "gridsLenHH" };

    inline const juce::String marblesRate   { "marblesRate" };
    inline const juce::String marblesJitter { "marblesJitter" };
    inline const juce::String marblesBias   { "marblesBias" };
    inline const juce::String marblesDejaVu { "marblesDejaVu" };
    inline const juce::String marblesLength { "marblesLength" };
    inline const juce::String marblesModel  { "marblesModel" };

    inline const juce::String modGridsX     { "modGridsX" };
    inline const juce::String modGridsY     { "modGridsY" };
    inline const juce::String modGridsChaos { "modGridsChaos" };
    inline const juce::String modGridsDens  { "modGridsDens" };

    inline const juce::String lfoRate   { "lfoRate" };
    inline const juce::String lfoShape  { "lfoShape" };

    inline const juce::String dlySync     { "dlySync" };
    inline const juce::String dlyDiv      { "dlyDiv" };
    inline const juce::String dlyTime     { "dlyTime" };
    inline const juce::String dlyFeedback { "dlyFeedback" };
    inline const juce::String dlyTone     { "dlyTone" };
    inline const juce::String dlyPingPong { "dlyPingPong" };
    inline const juce::String dlyTape     { "dlyTape" };
    inline const juce::String dlyReturn   { "dlyReturn" };

    inline const juce::String revDecay    { "revDecay" };
    inline const juce::String revBright   { "revBright" };
    inline const juce::String revPreDelay { "revPreDelay" };
    inline const juce::String revWidth    { "revWidth" };
    inline const juce::String revReturn   { "revReturn" };
}

// Per-voice parameter suffixes
namespace V {
    inline const char* engine  = "engine";
    inline const char* trig    = "trig";
    inline const char* choke   = "choke";
    inline const char* note    = "note";
    inline const char* tune    = "tune";
    inline const char* decay   = "decay";
    inline const char* tone    = "tone";
    inline const char* chr     = "char";
    inline const char* level   = "level";
    inline const char* pan     = "pan";
    inline const char* mute    = "mute";
    inline const char* solo    = "solo";
    inline const char* send1   = "sendDly";
    inline const char* send2   = "sendRev";
    inline const char* out     = "out";

    inline const char* eqOn    = "eqOn";
    inline const char* eqLoF   = "eqLoF";
    inline const char* eqLoG   = "eqLoG";
    inline const char* eqMidF  = "eqMidF";
    inline const char* eqMidQ  = "eqMidQ";
    inline const char* eqMidG  = "eqMidG";
    inline const char* eqHiF   = "eqHiF";
    inline const char* eqHiG   = "eqHiG";

    inline const char* drvOn   = "drvOn";
    inline const char* drive   = "drive";
    inline const char* drvTone = "drvTone";
    inline const char* drvTrim = "drvTrim";
    inline const char* drvMode = "drvMode";

    inline const char* cmpOn   = "cmpOn";
    inline const char* cmpThr  = "cmpThr";
    inline const char* cmpRat  = "cmpRatio";
    inline const char* cmpAtk  = "cmpAtk";
    inline const char* cmpRel  = "cmpRel";
    inline const char* cmpMak  = "cmpMakeup";
}

inline juce::String vid(int voice, const char* suffix) {
    return "v" + juce::String(voice + 1) + "_" + suffix;
}
inline juce::String modSrcId(int voice, int slot) { return "v" + juce::String(voice + 1) + "_m" + juce::String(slot + 1) + "Src"; }
inline juce::String modDstId(int voice, int slot) { return "v" + juce::String(voice + 1) + "_m" + juce::String(slot + 1) + "Dst"; }
inline juce::String modAmtId(int voice, int slot) { return "v" + juce::String(voice + 1) + "_m" + juce::String(slot + 1) + "Amt"; }

static const int kDefaultNotes[kNumVoices] = { 36, 38, 42, 46, 39, 45, 48, 49 };

inline const juce::StringArray& marblesRateNames() {
    static juce::StringArray s { "1/4x", "1/2x", "1x", "2x", "3x", "4x" };
    return s;
}
inline float marblesRateValue(int idx) {
    static const float v[] = { 0.25f, 0.5f, 1.0f, 2.0f, 3.0f, 4.0f };
    return v[juce::jlimit(0, 5, idx)];
}
inline int marblesRateIndexFor(float rate) {
    int best = 2; float bestD = 1e9f;
    for (int i = 0; i < 6; ++i) {
        float d = std::abs(std::log2(marblesRateValue(i)) - std::log2(juce::jmax(0.01f, rate)));
        if (d < bestD) { bestD = d; best = i; }
    }
    return best;
}

inline const juce::StringArray& lfoRateNames() {
    static juce::StringArray s { "1/16", "1/8", "1/4", "1/2", "1 bar", "2 bars", "4 bars", "8 bars" };
    return s;
}
inline double lfoRateBeats(int idx) {
    static const double v[] = { 0.25, 0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 32.0 };
    return v[juce::jlimit(0, 7, idx)];
}
inline const juce::StringArray& lfoShapeNames() {
    static juce::StringArray s { "Sine", "Triangle", "Ramp Up", "Ramp Down", "Square", "S&H" };
    return s;
}

// ---- Helpers -----------------------------------------------------------------------------
using APF = juce::AudioParameterFloat;
using APB = juce::AudioParameterBool;
using APC = juce::AudioParameterChoice;
using API = juce::AudioParameterInt;
using Range = juce::NormalisableRange<float>;

inline juce::String dbText(float v, int) {
    if (v <= -59.5f) return "-inf";
    return juce::String(v, 1) + " dB";
}
inline juce::String pctText(float v, int) { return juce::String(juce::roundToInt(v * 100.0f)) + "%"; }
inline juce::String bipolarPctText(float v, int) {
    int p = juce::roundToInt(v * 100.0f);
    return (p > 0 ? "+" : "") + juce::String(p) + "%";
}
inline juce::String hzText(float v, int) {
    return v >= 1000.0f ? juce::String(v / 1000.0f, 2) + " kHz" : juce::String(juce::roundToInt(v)) + " Hz";
}
inline juce::String msText(float v, int) {
    if (v >= 1000.0f) return juce::String(v / 1000.0f, 2) + " s";
    return (v < 10.0f ? juce::String(v, 1) : juce::String(juce::roundToInt(v))) + " ms";
}
inline juce::String panText(float v, int) {
    int p = juce::roundToInt(v * 100.0f);
    if (p == 0) return "C";
    return p < 0 ? "L" + juce::String(-p) : "R" + juce::String(p);
}

// On/off switches are two-state choices so hosts always restore exactly 0 or 1.
inline std::unique_ptr<juce::AudioParameterChoice> toggle(const juce::String& id, const juce::String& name, bool def) {
    return std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ id, 1 }, name, juce::StringArray{ "Off", "On" }, def ? 1 : 0);
}

inline Range skewed(float lo, float hi, float centre) {
    Range r(lo, hi);
    r.setSkewForCentre(centre);
    return r;
}

inline std::unique_ptr<APF> fl(const juce::String& id, const juce::String& name, Range r, float def,
                               std::function<juce::String(float, int)> toText = nullptr,
                               const juce::String& label = {}) {
    auto attrs = juce::AudioParameterFloatAttributes().withLabel(label);
    if (toText) attrs = attrs.withStringFromValueFunction(std::move(toText));
    return std::make_unique<APF>(juce::ParameterID{ id, 1 }, name, r, def, attrs);
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ---- Transport / master
    {
        auto g = std::make_unique<juce::AudioProcessorParameterGroup>("global", "Global", " | ");
        g->addChild(toggle(ID::run, "Sequencer Run", true));
        g->addChild(toggle(ID::hostSync, "Sync To Host", true));
        g->addChild(fl(ID::bpm, "Internal BPM", Range(30.0f, 300.0f, 0.1f), 120.0f,
                       [](float v, int) { return juce::String(v, 1) + " BPM"; }));
        g->addChild(fl(ID::master, "Master Volume", Range(-60.0f, 6.0f, 0.1f), -3.0f, dbText));
        layout.add(std::move(g));
    }

    // ---- Grids
    {
        auto g = std::make_unique<juce::AudioProcessorParameterGroup>("grids", "Grids", " | ");
        g->addChild(fl(ID::gridsX, "Grids Map X", Range(0, 1), 0.5f, pctText));
        g->addChild(fl(ID::gridsY, "Grids Map Y", Range(0, 1), 0.5f, pctText));
        g->addChild(fl(ID::gridsChaos, "Grids Chaos", Range(0, 1), 0.0f, pctText));
        g->addChild(fl(ID::gridsDensBD, "Grids BD Density", Range(0, 1), 0.6f, pctText));
        g->addChild(fl(ID::gridsDensSD, "Grids SD Density", Range(0, 1), 0.5f, pctText));
        g->addChild(fl(ID::gridsDensHH, "Grids HH Density", Range(0, 1), 0.7f, pctText));
        g->addChild(std::make_unique<APC>(juce::ParameterID{ ID::gridsMode, 1 }, "Grids Mode",
                                          juce::StringArray{ "Topographic", "Euclidean" }, 0));
        g->addChild(fl(ID::gridsSwing, "Swing", Range(0.0f, 0.75f), 0.0f, pctText));
        g->addChild(std::make_unique<API>(juce::ParameterID{ ID::gridsLenBD, 1 }, "Euclid BD Length", 1, 32, 16));
        g->addChild(std::make_unique<API>(juce::ParameterID{ ID::gridsLenSD, 1 }, "Euclid SD Length", 1, 32, 16));
        g->addChild(std::make_unique<API>(juce::ParameterID{ ID::gridsLenHH, 1 }, "Euclid HH Length", 1, 32, 16));
        g->addChild(fl(ID::modGridsX, "X1 > Map X", Range(-1, 1), 0.0f, bipolarPctText));
        g->addChild(fl(ID::modGridsY, "X2 > Map Y", Range(-1, 1), 0.0f, bipolarPctText));
        g->addChild(fl(ID::modGridsChaos, "X3 > Chaos", Range(-1, 1), 0.0f, bipolarPctText));
        g->addChild(fl(ID::modGridsDens, "t1 > Density", Range(-1, 1), 0.0f, bipolarPctText));
        layout.add(std::move(g));
    }

    // ---- Marbles
    {
        auto g = std::make_unique<juce::AudioProcessorParameterGroup>("marbles", "Marbles", " | ");
        g->addChild(std::make_unique<APC>(juce::ParameterID{ ID::marblesRate, 1 }, "Marbles Rate", marblesRateNames(), 2));
        g->addChild(fl(ID::marblesJitter, "Marbles Jitter", Range(0, 1), 0.0f, pctText));
        g->addChild(fl(ID::marblesBias, "Marbles Bias", Range(0, 1), 0.5f, pctText));
        g->addChild(fl(ID::marblesDejaVu, "Marbles Deja Vu", Range(0, 1), 0.0f, pctText));
        g->addChild(std::make_unique<API>(juce::ParameterID{ ID::marblesLength, 1 }, "Marbles Loop Length", 1, 16, 16));
        g->addChild(std::make_unique<APC>(juce::ParameterID{ ID::marblesModel, 1 }, "Marbles Model",
                                          juce::StringArray{ "Coin Toss", "Polyrhythm", "Drums" }, 0));
        g->addChild(std::make_unique<APC>(juce::ParameterID{ ID::lfoRate, 1 }, "LFO Rate", lfoRateNames(), 4));
        g->addChild(std::make_unique<APC>(juce::ParameterID{ ID::lfoShape, 1 }, "LFO Shape", lfoShapeNames(), 0));
        layout.add(std::move(g));
    }

    // ---- FX
    {
        auto g = std::make_unique<juce::AudioProcessorParameterGroup>("fx", "Effects", " | ");
        juce::StringArray divs;
        for (int i = 0; i < FxDelay::NUM_DIVISIONS; ++i) divs.add(FxDelay::getDivisionName(i));
        g->addChild(toggle(ID::dlySync, "Delay Sync", true));
        g->addChild(std::make_unique<APC>(juce::ParameterID{ ID::dlyDiv, 1 }, "Delay Division", divs, FxDelay::DIV_1_8_D));
        g->addChild(fl(ID::dlyTime, "Delay Time", skewed(5.0f, 2000.0f, 300.0f), 375.0f, msText));
        g->addChild(fl(ID::dlyFeedback, "Delay Feedback", Range(0.0f, 0.95f), 0.4f, pctText));
        g->addChild(fl(ID::dlyTone, "Delay Tone", Range(0, 1), 0.5f, pctText));
        g->addChild(fl(ID::dlyPingPong, "Delay Ping-Pong", Range(0, 1), 0.7f, pctText));
        g->addChild(fl(ID::dlyTape, "Delay Tape", Range(0, 1), 0.5f, pctText));
        g->addChild(fl(ID::dlyReturn, "Delay Return", Range(-60.0f, 6.0f, 0.1f), -14.0f, dbText));

        g->addChild(fl(ID::revDecay, "Reverb Decay", Range(0, 1), 0.6f,
                       [](float v, int) { return juce::String(FxReverb::decayToSeconds(v), 1) + " s"; }));
        g->addChild(fl(ID::revBright, "Reverb Brightness", Range(0, 1), 0.45f, pctText));
        g->addChild(fl(ID::revPreDelay, "Reverb Pre-Delay", Range(0.0f, 200.0f, 0.1f), 10.0f, msText));
        g->addChild(fl(ID::revWidth, "Reverb Width", Range(0, 1), 0.85f, pctText));
        g->addChild(fl(ID::revReturn, "Reverb Return", Range(-60.0f, 6.0f, 0.1f), -16.0f, dbText));
        layout.add(std::move(g));
    }

    // ---- Voices
    juce::StringArray engines, trigs, chokes, srcs, dsts, outs;
    for (int i = 0; i < kNumOutputs; ++i) outs.add(outputName(i));
    for (int i = 0; i < NUM_ENGINES; ++i) engines.add(getEngineInfo(i).name);
    for (int i = 0; i < NUM_TRIG_SOURCES; ++i) trigs.add(getTriggerSourceName(i));
    chokes.addArray({ "None", "Group A", "Group B", "Group C", "Group D" });
    for (int i = 0; i < NUM_MOD_SOURCES; ++i) srcs.add(getModSourceName(i));
    for (int i = 0; i < NUM_MOD_DESTINATIONS; ++i) dsts.add(getModDestName(i));

    for (int v = 0; v < kNumVoices; ++v) {
        const juce::String pre = "Ch" + juce::String(v + 1) + " ";
        auto g = std::make_unique<juce::AudioProcessorParameterGroup>("voice" + juce::String(v + 1), "Channel " + juce::String(v + 1), " | ");

        g->addChild(std::make_unique<APC>(juce::ParameterID{ vid(v, V::engine), 1 }, pre + "Engine", engines, 0));
        g->addChild(std::make_unique<APC>(juce::ParameterID{ vid(v, V::trig), 1 }, pre + "Trigger", trigs, 0));
        g->addChild(std::make_unique<APC>(juce::ParameterID{ vid(v, V::choke), 1 }, pre + "Choke Group", chokes, 0));
        g->addChild(std::make_unique<API>(juce::ParameterID{ vid(v, V::note), 1 }, pre + "MIDI Note", 0, 127, kDefaultNotes[v],
            juce::AudioParameterIntAttributes().withStringFromValueFunction([](int n, int) {
                return juce::MidiMessage::getMidiNoteName(n, true, true, 3) + " (" + juce::String(n) + ")";
            })));

        g->addChild(fl(vid(v, V::tune), pre + "Tune", Range(-36.0f, 36.0f, 0.01f), 0.0f,
                       [](float x, int) { return (x > 0.005f ? "+" : "") + juce::String(x, 1) + " st"; }));
        g->addChild(fl(vid(v, V::decay), pre + "Decay", Range(0, 1), 0.5f, pctText));
        g->addChild(fl(vid(v, V::tone), pre + "Tone", Range(0, 1), 0.5f, pctText));
        g->addChild(fl(vid(v, V::chr), pre + "Character", Range(0, 1), 0.5f, pctText));

        g->addChild(fl(vid(v, V::level), pre + "Level", Range(-60.0f, 6.0f, 0.1f), -60.0f, dbText));
        g->addChild(fl(vid(v, V::pan), pre + "Pan", Range(-1.0f, 1.0f), 0.0f, panText));
        g->addChild(toggle(vid(v, V::mute), pre + "Mute", false));
        g->addChild(toggle(vid(v, V::solo), pre + "Solo", false));
        g->addChild(fl(vid(v, V::send1), pre + "Delay Send", Range(-60.0f, 6.0f, 0.1f), -60.0f, dbText));
        g->addChild(fl(vid(v, V::send2), pre + "Reverb Send", Range(-60.0f, 6.0f, 0.1f), -60.0f, dbText));
        g->addChild(std::make_unique<APC>(juce::ParameterID{ vid(v, V::out), 1 }, pre + "Output", outs, 0));

        g->addChild(toggle(vid(v, V::eqOn), pre + "EQ On", true));
        g->addChild(fl(vid(v, V::eqLoF), pre + "EQ Low Freq", skewed(30.0f, 600.0f, 120.0f), 100.0f, hzText));
        g->addChild(fl(vid(v, V::eqLoG), pre + "EQ Low Gain", Range(-18.0f, 18.0f, 0.1f), 0.0f, dbText));
        g->addChild(fl(vid(v, V::eqMidF), pre + "EQ Mid Freq", skewed(200.0f, 8000.0f, 1200.0f), 1200.0f, hzText));
        g->addChild(fl(vid(v, V::eqMidQ), pre + "EQ Mid Q", skewed(0.3f, 8.0f, 1.0f), 1.0f,
                       [](float x, int) { return juce::String(x, 2); }));
        g->addChild(fl(vid(v, V::eqMidG), pre + "EQ Mid Gain", Range(-18.0f, 18.0f, 0.1f), 0.0f, dbText));
        g->addChild(fl(vid(v, V::eqHiF), pre + "EQ High Freq", skewed(1500.0f, 16000.0f, 6000.0f), 8000.0f, hzText));
        g->addChild(fl(vid(v, V::eqHiG), pre + "EQ High Gain", Range(-18.0f, 18.0f, 0.1f), 0.0f, dbText));

        g->addChild(toggle(vid(v, V::drvOn), pre + "Drive On", true));
        g->addChild(fl(vid(v, V::drive), pre + "Drive", Range(0, 1), 0.0f, pctText));
        g->addChild(fl(vid(v, V::drvTone), pre + "Drive Tone", Range(0, 1), 0.5f,
                       [](float x, int) {
                           if (std::abs(x - 0.5f) < 0.01f) return juce::String("Neutral");
                           return x < 0.5f ? "Dark " + juce::String(juce::roundToInt((0.5f - x) * 200)) + "%"
                                           : "Bright " + juce::String(juce::roundToInt((x - 0.5f) * 200)) + "%";
                       }));
        g->addChild(fl(vid(v, V::drvTrim), pre + "Drive Trim", Range(-12.0f, 12.0f, 0.1f), 0.0f, dbText));
        g->addChild(std::make_unique<APC>(juce::ParameterID{ vid(v, V::drvMode), 1 }, pre + "Drive Mode",
                                          juce::StringArray{ "Tape", "Tube", "Clip" }, 0));

        g->addChild(toggle(vid(v, V::cmpOn), pre + "Comp On", false));
        g->addChild(fl(vid(v, V::cmpThr), pre + "Comp Threshold", Range(-40.0f, 0.0f, 0.1f), -14.0f, dbText));
        g->addChild(fl(vid(v, V::cmpRat), pre + "Comp Ratio", skewed(1.0f, 20.0f, 4.0f), 4.0f,
                       [](float x, int) { return juce::String(x, 1) + ":1"; }));
        g->addChild(fl(vid(v, V::cmpAtk), pre + "Comp Attack", skewed(0.1f, 100.0f, 5.0f), 5.0f, msText));
        g->addChild(fl(vid(v, V::cmpRel), pre + "Comp Release", skewed(5.0f, 1000.0f, 80.0f), 60.0f, msText));
        g->addChild(fl(vid(v, V::cmpMak), pre + "Comp Makeup", Range(0.0f, 24.0f, 0.1f), 0.0f, dbText));

        for (int s = 0; s < kNumModSlots; ++s) {
            const juce::String sp = pre + "Mod " + juce::String(s + 1) + " ";
            g->addChild(std::make_unique<APC>(juce::ParameterID{ modSrcId(v, s), 1 }, sp + "Source", srcs, 0));
            g->addChild(std::make_unique<APC>(juce::ParameterID{ modDstId(v, s), 1 }, sp + "Destination", dsts, s == 0 ? 0 : (s == 1 ? 1 : 3)));
            g->addChild(fl(modAmtId(v, s), sp + "Amount", Range(-1.0f, 1.0f), 0.0f, bipolarPctText));
        }

        layout.add(std::move(g));
    }

    return layout;
}

} // namespace Params
} // namespace Mutadrums
