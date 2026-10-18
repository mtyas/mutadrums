#pragma once
#include <string>
#include <vector>
#include <array>

namespace MutableDrums {

struct ChannelPreset {
    int engine{ 0 };
    float tune{ 0.0f };
    float decay{ 0.5f };
    float tone{ 0.5f };
    float timbre{ 0.5f };
    float morph{ 0.5f };
    int triggerSource{ 1 };
    int chokeGroup{ 0 };

    // Modulation slots. Destinations (format v2): 0 Tune, 1 Decay, 2 Tone, 3 Character, 4 Level, 5 Pan
    // Sources: 0 Off, 1-3 Marbles X1-X3, 4 Velocity, 5 Random, 6 LFO, 7 Bar Ramp
    int modSource1{ 0 };
    int modDest1{ 0 };       // Tune
    float modAmount1{ 0.0f };
    int modSource2{ 0 };     // Off
    int modDest2{ 1 };       // Decay
    float modAmount2{ 0.0f };
    int modSource3{ 0 };     // Off
    int modDest3{ 3 };       // Character
    float modAmount3{ 0.0f };
    int midiNote{ -1 };      // -1 = channel default (GM drum map)

    // Legacy fields for backward compatibility
    float modXAmount{ 0.0f };
    int modXTarget{ 0 };

    // Calibrated studio default: unused channels are silent (-60 dB)
    float volumeDB{ -60.0f };
    float pan{ 0.0f };
    float eqLowFreq{ 100.0f };
    float eqLowGain{ 0.0f };
    float eqMidFreq{ 1200.0f };
    float eqMidQ{ 1.0f };
    float eqMidGain{ 0.0f };
    float eqHighGain{ 0.0f };
    float eqHighFreq{ 8000.0f };
    bool eqBypass{ false };

    float drive{ 0.0f };
    float driveTone{ 0.5f };
    float driveTrimDB{ 0.0f };
    int driveMode{ 0 };
    bool driveBypass{ false };

    float compThresh{ -14.0f };
    float compRatio{ 4.0f };
    float compAttackMs{ 5.0f };
    float compReleaseMs{ 60.0f };
    float compMakeupDB{ 0.0f };
    bool compBypass{ false };

    float aux1SendDB{ -60.0f }; // Delay
    float aux2SendDB{ -60.0f }; // Reverb
};

struct PresetData {
    std::string name;
    std::string category;
    std::string author;
    std::string description;
    bool isFactory{ false };
    int formatVersion{ 1 }; // 1 = original FX calibration (factory + old saves), 2 = current

    // Grids Sequencer
    float gridsX{ 0.5f };
    float gridsY{ 0.5f };
    float gridsChaos{ 0.0f };
    float gridsDensity[3]{ 0.6f, 0.5f, 0.7f };
    int gridsEuclideanLength[3]{ 16, 16, 16 };
    int gridsOutputMode{ 0 }; // 0: Drums, 1: Euclidean
    float gridsSwing{ 0.0f };
    int lfoRate{ 4 };   // 1 bar
    int lfoShape{ 0 };  // Sine

    // Marbles Sequencer
    float marblesRate{ 1.0f };
    float marblesJitter{ 0.0f };
    float marblesBias{ 0.5f };
    float marblesDejaVu{ 0.0f };
    int marblesLoopLength{ 16 };
    int marblesModel{ 0 };

    // Marbles -> Grids Modulation
    float gridsModMapX{ 0.0f };
    float gridsModMapY{ 0.0f };
    float gridsModChaos{ 0.0f };
    float gridsModDensity{ 0.0f };
    int gridsModSourceMapX{ 1 };    // 1: X1
    int gridsModSourceMapY{ 2 };    // 2: X2
    int gridsModSourceChaos{ 3 };   // 3: X3
    int gridsModSourceDensity{ 4 }; // 4: t1

    // Tempo & Host Sync
    float bpm{ 120.0f };
    bool syncToHost{ true };

    // FX Delay (Aux 1) - Tape Delay with BPM Sync
    float delayTimeMs{ 375.0f };
    float delayFeedback{ 0.40f };
    float delayTone{ 0.5f };
    float delayPingPong{ 0.7f };
    float delayReturnDB{ -14.0f };
    bool delaySync{ true };
    int delayDivision{ 6 }; // 1/8D
    float delayWarmth{ 0.6f };

    // FX Reverb (Aux 2) - Dattorro Plate Reverb
    float reverbDecay{ 0.60f };
    float reverbDamp{ 0.45f };
    float reverbPreDelayMs{ 10.0f };
    float reverbWidth{ 0.85f };
    float reverbReturnDB{ -16.0f };

    // Master Headroom (-3.0 dB gives clean -6 to -3 dBFS DAW headroom)
    float masterVolumeDB{ -3.0f };

    std::array<ChannelPreset, 8> channels;
};

class FactoryPresets {
public:
    static std::vector<PresetData> getFactoryPresets() {
        std::vector<PresetData> presets;

        // =========================================================================
        // CATEGORY: TECHNO (6 PRESETS)
        // =========================================================================

        // 1. Techno Berlin Peaktime (130 BPM)
        {
            PresetData p;
            p.name = "Techno 909 Peaktime";
            p.category = "Techno";
            p.author = "MutableDSP";
            p.description = "Driving peaktime Berlin techno with punchy 909 kick, snappy resonant snare, driving 16th hats, and parallel drive.";
            p.bpm = 130.0f;
            p.gridsX = 0.82f; p.gridsY = 0.72f; p.gridsChaos = 0.12f;
            p.gridsDensity[0] = 0.75f; p.gridsDensity[1] = 0.65f; p.gridsDensity[2] = 0.85f;
            p.marblesDejaVu = 0.8f; p.marblesBias = 0.6f;

            // Voice 0: Kick (Plaits Synth BD)
            p.channels[0].engine = 1; p.channels[0].tune = 0.0f; p.channels[0].decay = 0.52f;
            p.channels[0].tone = 0.65f; p.channels[0].morph = 0.35f; p.channels[0].drive = 0.25f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f; p.channels[0].compThresh = -14.0f;

            // Voice 1: Snare (Plaits Synth SD)
            p.channels[1].engine = 3; p.channels[1].tune = 2.0f; p.channels[1].decay = 0.42f;
            p.channels[1].timbre = 0.75f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f;
            p.channels[1].aux2SendDB = -18.0f;

            // Voice 2: Closed Hat (Plaits Hi-Hat)
            p.channels[2].engine = 4; p.channels[2].tune = 4.0f; p.channels[2].decay = 0.14f;
            p.channels[2].triggerSource = 3; p.channels[2].chokeGroup = 1; p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.25f;

            // Voice 3: Open Hat (Plaits Open Hat)
            p.channels[3].engine = 5; p.channels[3].tune = 4.0f; p.channels[3].decay = 0.55f;
            p.channels[3].triggerSource = 6; p.channels[3].chokeGroup = 1; p.channels[3].volumeDB = -14.0f; p.channels[3].pan = 0.28f;

            // Voice 4: Clap (Plaits Particle Clap)
            p.channels[4].engine = 7; p.channels[4].decay = 0.32f; p.channels[4].triggerSource = 5;
            p.channels[4].volumeDB = -11.0f; p.channels[4].aux1SendDB = -20.0f;

            // Voice 5: Modal Perc (Plaits Modal)
            p.channels[5].engine = 6; p.channels[5].tune = -4.0f; p.channels[5].decay = 0.38f;
            p.channels[5].triggerSource = 4; p.channels[5].volumeDB = -12.0f; p.channels[5].pan = 0.35f;
            p.channels[5].modSource1 = 1; p.channels[5].modDest1 = 3; p.channels[5].modAmount1 = 0.35f;

            // Voice 6: Sub Thump (Braids Kick)
            p.channels[6].engine = 8; p.channels[6].tune = -3.0f; p.channels[6].decay = 0.28f;
            p.channels[6].triggerSource = 1; p.channels[6].volumeDB = -15.0f;

            // Voice 7: Dark Ride (Braids Cymbal)
            p.channels[7].engine = 10; p.channels[7].decay = 0.65f; p.channels[7].triggerSource = 0;
            p.channels[7].volumeDB = -16.0f; p.channels[7].aux2SendDB = -16.0f;

            presets.push_back(p);
        }

        // 2. Techno Raw Acid 303 (134 BPM)
        {
            PresetData p;
            p.name = "Techno Raw Acid 303";
            p.category = "Techno";
            p.author = "MutableDSP";
            p.description = "Gritty, overdriven underground warehouse acid with saturated transient kick, snappy claps, and Marbles chaos modulation.";
            p.bpm = 134.0f;
            p.gridsX = 0.78f; p.gridsY = 0.60f; p.gridsChaos = 0.25f;
            p.gridsDensity[0] = 0.8f; p.gridsDensity[1] = 0.6f; p.gridsDensity[2] = 0.85f;
            p.marblesRate = 1.0f; p.marblesJitter = 0.2f; p.marblesDejaVu = 0.5f;

            p.channels[0].engine = 1; p.channels[0].tune = 1.0f; p.channels[0].decay = 0.45f;
            p.channels[0].tone = 0.75f; p.channels[0].drive = 0.4f; p.channels[0].volumeDB = -7.5f;
            p.channels[0].triggerSource = 1;

            p.channels[1].engine = 3; p.channels[1].decay = 0.38f; p.channels[1].timbre = 0.85f;
            p.channels[1].drive = 0.3f; p.channels[1].volumeDB = -8.5f; p.channels[1].triggerSource = 2;

            p.channels[2].engine = 4; p.channels[2].decay = 0.12f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.5f; p.channels[2].pan = -0.3f; p.channels[2].chokeGroup = 1;

            p.channels[3].engine = 5; p.channels[3].decay = 0.5f; p.channels[3].triggerSource = 5;
            p.channels[3].volumeDB = -14.0f; p.channels[3].pan = 0.3f; p.channels[3].chokeGroup = 1;

            p.channels[4].engine = 7; p.channels[4].decay = 0.28f; p.channels[4].volumeDB = -11.5f;
            p.channels[4].triggerSource = 4; p.channels[4].aux1SendDB = -18.0f;

            presets.push_back(p);
        }

        // 3. Techno Dub Subspace (124 BPM)
        {
            PresetData p;
            p.name = "Techno Dub Subspace";
            p.category = "Techno";
            p.author = "MutableDSP";
            p.description = "Deep warm sub bass kick, tape saturated ping-pong delay and diffuse atmospheric reverb.";
            p.bpm = 124.0f;
            p.gridsX = 0.25f; p.gridsY = 0.82f; p.gridsChaos = 0.05f;
            p.gridsDensity[0] = 0.7f; p.gridsDensity[1] = 0.42f; p.gridsDensity[2] = 0.65f;
            p.delayFeedback = 0.65f; p.delayTimeMs = 468.0f; p.delayReturnDB = -12.0f;
            p.reverbDecay = 0.85f; p.reverbReturnDB = -14.0f;

            p.channels[0].engine = 0; p.channels[0].decay = 0.75f; p.channels[0].tone = 0.32f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 2; p.channels[1].decay = 0.35f; p.channels[1].triggerSource = 2;
            p.channels[1].volumeDB = -8.5f; p.channels[1].aux1SendDB = -12.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.15f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.5f; p.channels[2].aux1SendDB = -18.0f;

            p.channels[3].engine = 6; p.channels[3].tune = 3.0f; p.channels[3].decay = 0.6f;
            p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -12.0f; p.channels[3].aux1SendDB = -10.0f; p.channels[3].aux2SendDB = -12.0f;

            presets.push_back(p);
        }

        // 4. Techno Minimal Micro (126 BPM)
        {
            PresetData p;
            p.name = "Techno Minimal Micro";
            p.category = "Techno";
            p.author = "MutableDSP";
            p.description = "Sleek, tight, clicking micro-percussions with precise transient response and clinical groove.";
            p.bpm = 126.0f;
            p.gridsX = 0.45f; p.gridsY = 0.32f; p.gridsChaos = 0.08f;
            p.gridsDensity[0] = 0.65f; p.gridsDensity[1] = 0.45f; p.gridsDensity[2] = 0.72f;

            p.channels[0].engine = 0; p.channels[0].decay = 0.36f; p.channels[0].tone = 0.6f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 3; p.channels[1].decay = 0.22f; p.channels[1].timbre = 0.4f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.5f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.08f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.5f; p.channels[2].pan = -0.2f;

            p.channels[3].engine = 6; p.channels[3].tune = 12.0f; p.channels[3].decay = 0.16f;
            p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -13.0f; p.channels[3].pan = 0.3f;

            p.channels[4].engine = 7; p.channels[4].decay = 0.18f; p.channels[4].triggerSource = 5;
            p.channels[4].volumeDB = -12.5f; p.channels[4].pan = -0.25f;

            presets.push_back(p);
        }

        // 5. Techno Dark Industrial (136 BPM)
        {
            PresetData p;
            p.name = "Techno Dark Industrial";
            p.category = "Techno";
            p.author = "MutableDSP";
            p.description = "Crushing, distorted industrial rhythm section with overdriven Braids kicks and aggressive tube saturation.";
            p.bpm = 136.0f;
            p.gridsX = 0.88f; p.gridsY = 0.84f; p.gridsChaos = 0.32f;
            p.gridsDensity[0] = 0.82f; p.gridsDensity[1] = 0.70f; p.gridsDensity[2] = 0.88f;

            p.channels[0].engine = 1; p.channels[0].drive = 0.55f; p.channels[0].volumeDB = -8.0f;
            p.channels[0].compThresh = -18.0f; p.channels[0].compRatio = 6.0f; p.channels[0].triggerSource = 1;

            p.channels[1].engine = 9; p.channels[1].drive = 0.45f; p.channels[1].volumeDB = -9.0f;
            p.channels[1].triggerSource = 2; p.channels[1].compThresh = -16.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.15f; p.channels[2].volumeDB = -14.0f;
            p.channels[2].triggerSource = 3;

            p.channels[3].engine = 10; p.channels[3].decay = 0.45f; p.channels[3].drive = 0.35f;
            p.channels[3].volumeDB = -16.0f; p.channels[3].triggerSource = 5;

            p.channels[4].engine = 7; p.channels[4].drive = 0.4f; p.channels[4].volumeDB = -12.0f;
            p.channels[4].triggerSource = 6;

            presets.push_back(p);
        }

        // 6. Techno Detroit Soul (128 BPM)
        {
            PresetData p;
            p.name = "Techno Detroit Soul";
            p.category = "Techno";
            p.author = "MutableDSP";
            p.description = "Warm analog drum machine architecture with subtle organic swing, open hats and chordal modal percussions.";
            p.bpm = 128.0f;
            p.gridsX = 0.62f; p.gridsY = 0.54f; p.gridsChaos = 0.10f;
            p.gridsSwing = 0.15f;

            p.channels[0].engine = 0; p.channels[0].tune = -2.0f; p.channels[0].decay = 0.62f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 2; p.channels[1].decay = 0.42f; p.channels[1].timbre = 0.65f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f; p.channels[1].aux2SendDB = -16.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.16f; p.channels[2].triggerSource = 3;
            p.channels[2].chokeGroup = 1; p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.22f;

            p.channels[3].engine = 5; p.channels[3].decay = 0.62f; p.channels[3].triggerSource = 6;
            p.channels[3].chokeGroup = 1; p.channels[3].volumeDB = -14.0f; p.channels[3].pan = 0.25f;

            p.channels[4].engine = 6; p.channels[4].tune = 7.0f; p.channels[4].decay = 0.5f;
            p.channels[4].triggerSource = 4; p.channels[4].volumeDB = -12.5f; p.channels[4].aux1SendDB = -16.0f;

            presets.push_back(p);
        }

        // =========================================================================
        // CATEGORY: ELECTRO & SYNTH (5 PRESETS)
        // =========================================================================

        // 7. Electro 808 Classic (125 BPM)
        {
            PresetData p;
            p.name = "Electro 808 Classic";
            p.category = "Electro";
            p.author = "MutableDSP";
            p.description = "Deep analog sub 808 kick with dual resonant snare, particle claps, and ping-pong sync delay.";
            p.bpm = 125.0f;
            p.gridsX = 0.35f; p.gridsY = 0.40f; p.gridsChaos = 0.05f;
            p.gridsDensity[0] = 0.7f; p.gridsDensity[1] = 0.55f; p.gridsDensity[2] = 0.8f;
            p.delayTimeMs = 360.0f; p.delayReturnDB = -15.0f; p.reverbReturnDB = -18.0f;

            p.channels[0].engine = 0; p.channels[0].tune = -3.0f; p.channels[0].decay = 0.72f;
            p.channels[0].tone = 0.42f; p.channels[0].morph = 0.25f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.0f;

            p.channels[1].engine = 2; p.channels[1].decay = 0.42f; p.channels[1].timbre = 0.72f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f; p.channels[1].aux2SendDB = -18.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.18f; p.channels[2].triggerSource = 3;
            p.channels[2].chokeGroup = 1; p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.2f;

            p.channels[3].engine = 5; p.channels[3].decay = 0.65f; p.channels[3].triggerSource = 6;
            p.channels[3].chokeGroup = 1; p.channels[3].volumeDB = -13.5f; p.channels[3].pan = 0.22f;

            p.channels[4].engine = 7; p.channels[4].decay = 0.38f; p.channels[4].triggerSource = 5;
            p.channels[4].volumeDB = -11.0f; p.channels[4].aux1SendDB = -16.0f;

            p.channels[5].engine = 11; p.channels[5].tune = 7.0f; p.channels[5].decay = 0.48f;
            p.channels[5].triggerSource = 4; p.channels[5].volumeDB = -13.0f; p.channels[5].aux2SendDB = -16.0f;

            presets.push_back(p);
        }

        // 8. Electro Cyberpunk 2088 (128 BPM)
        {
            PresetData p;
            p.name = "Electro Cyberpunk 2088";
            p.category = "Electro";
            p.author = "MutableDSP";
            p.description = "Futuristic cybernetic rhythm section with metallic transient snap, modulated morph, and stereo widening.";
            p.bpm = 128.0f;
            p.gridsX = 0.75f; p.gridsY = 0.45f; p.gridsChaos = 0.18f;

            p.channels[0].engine = 1; p.channels[0].decay = 0.48f; p.channels[0].drive = 0.35f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 3; p.channels[1].decay = 0.36f; p.channels[1].timbre = 0.8f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.5f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.12f; p.channels[2].volumeDB = -13.5f;
            p.channels[2].triggerSource = 3;

            p.channels[3].engine = 6; p.channels[3].tune = 9.0f; p.channels[3].decay = 0.35f;
            p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -12.0f; p.channels[3].aux1SendDB = -14.0f;

            p.channels[4].engine = 11; p.channels[4].tune = 14.0f; p.channels[4].decay = 0.6f;
            p.channels[4].triggerSource = 5; p.channels[4].volumeDB = -13.5f; p.channels[4].aux2SendDB = -15.0f;

            presets.push_back(p);
        }

        // 9. Electro Miami Bass Slam (130 BPM)
        {
            PresetData p;
            p.name = "Electro Miami Bass Slam";
            p.category = "Electro";
            p.author = "MutableDSP";
            p.description = "Heavy 808 sub kick drop with syncopated rimshots, fast rolling hats and vintage Florida electro vibe.";
            p.bpm = 130.0f;
            p.gridsX = 0.52f; p.gridsY = 0.38f; p.gridsChaos = 0.10f;

            p.channels[0].engine = 0; p.channels[0].tune = -4.0f; p.channels[0].decay = 0.85f;
            p.channels[0].tone = 0.35f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.0f;

            p.channels[1].engine = 2; p.channels[1].tune = 1.0f; p.channels[1].decay = 0.32f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.5f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.10f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.2f;

            p.channels[3].engine = 5; p.channels[3].decay = 0.45f; p.channels[3].triggerSource = 6;
            p.channels[3].volumeDB = -14.0f; p.channels[3].pan = 0.2f;

            p.channels[4].engine = 7; p.channels[4].decay = 0.28f; p.channels[4].triggerSource = 5;
            p.channels[4].volumeDB = -11.5f;

            presets.push_back(p);
        }

        // 10. Electro Synthwave Cruise (118 BPM)
        {
            PresetData p;
            p.name = "Electro Synthwave Cruise";
            p.category = "Electro";
            p.author = "MutableDSP";
            p.description = "Retro 80s gated snare, punchy disco kick, chorused tom fills and lush analog tape warmth.";
            p.bpm = 118.0f;
            p.gridsX = 0.60f; p.gridsY = 0.50f; p.gridsChaos = 0.06f;
            p.reverbDecay = 0.70f; p.reverbReturnDB = -14.0f;

            p.channels[0].engine = 1; p.channels[0].tune = -1.0f; p.channels[0].decay = 0.55f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 3; p.channels[1].decay = 0.50f; p.channels[1].timbre = 0.82f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f; p.channels[1].aux2SendDB = -14.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.14f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.0f;

            p.channels[3].engine = 6; p.channels[3].tune = -2.0f; p.channels[3].decay = 0.45f;
            p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -12.0f; p.channels[3].aux2SendDB = -16.0f;

            presets.push_back(p);
        }

        // 11. Electro Neon Kraftwerk (122 BPM)
        {
            PresetData p;
            p.name = "Electro Neon Kraftwerk";
            p.category = "Electro";
            p.author = "MutableDSP";
            p.description = "Precision electronic woodblocks, robotic synthesized ticks, and Euclidean rhythm geometry.";
            p.bpm = 122.0f;
            p.gridsOutputMode = 1; // Euclidean
            p.gridsEuclideanLength[0] = 16; p.gridsEuclideanLength[1] = 16; p.gridsEuclideanLength[2] = 16;
            p.gridsDensity[0] = 0.55f; p.gridsDensity[1] = 0.45f; p.gridsDensity[2] = 0.65f;

            p.channels[0].engine = 0; p.channels[0].decay = 0.4f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -8.0f;
            p.channels[1].engine = 2; p.channels[1].decay = 0.28f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -9.0f;
            p.channels[2].engine = 6; p.channels[2].tune = 10.0f; p.channels[2].decay = 0.18f; p.channels[2].triggerSource = 3; p.channels[2].volumeDB = -12.5f;
            p.channels[3].engine = 11; p.channels[3].tune = 12.0f; p.channels[3].decay = 0.25f; p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -13.0f;

            presets.push_back(p);
        }

        // =========================================================================
        // CATEGORY: IDM & GLITCH (5 PRESETS)
        // =========================================================================

        // 12. IDM Warp Generative (120 BPM)
        {
            PresetData p;
            p.name = "IDM Warp Generative";
            p.category = "IDM";
            p.author = "MutableDSP";
            p.description = "Complex generative IDM beats with Marbles Bernoulli coin toss, modal inharmonicities, and modulated timbres.";
            p.bpm = 120.0f;
            p.gridsX = 0.65f; p.gridsY = 0.25f; p.gridsChaos = 0.42f;
            p.marblesModel = 0; p.marblesDejaVu = 0.65f; p.marblesBias = 0.45f;
            p.delayFeedback = 0.55f; p.delayTimeMs = 320.0f; p.delayReturnDB = -14.0f;

            p.channels[0].engine = 1; p.channels[0].decay = 0.38f; p.channels[0].triggerSource = 4;
            p.channels[0].volumeDB = -7.5f; p.channels[0].modSource1 = 1; p.channels[0].modDest1 = 0; p.channels[0].modAmount1 = 0.25f;

            p.channels[1].engine = 9; p.channels[1].decay = 0.32f; p.channels[1].triggerSource = 5;
            p.channels[1].volumeDB = -8.5f; p.channels[1].aux1SendDB = -16.0f;

            p.channels[2].engine = 6; p.channels[2].tune = 5.0f; p.channels[2].morph = 0.8f;
            p.channels[2].triggerSource = 6; p.channels[2].volumeDB = -12.0f; p.channels[2].aux1SendDB = -12.0f;
            p.channels[2].modSource1 = 2; p.channels[2].modDest1 = 3; p.channels[2].modAmount1 = 0.4f;

            p.channels[3].engine = 7; p.channels[3].decay = 0.25f; p.channels[3].triggerSource = 3;
            p.channels[3].volumeDB = -13.0f; p.channels[3].pan = -0.3f;

            p.channels[4].engine = 10; p.channels[4].decay = 0.42f; p.channels[4].triggerSource = 2;
            p.channels[4].volumeDB = -15.0f; p.channels[4].aux2SendDB = -15.0f;

            presets.push_back(p);
        }

        // 13. IDM Autechre Inharmonic (132 BPM)
        {
            PresetData p;
            p.name = "IDM Autechre Inharmonic";
            p.category = "IDM";
            p.author = "MutableDSP";
            p.description = "Metallic modal resonators with sharp inharmonic frequency spectra, asymmetric overdrive, and chaotic stereo scatter.";
            p.bpm = 132.0f;
            p.gridsX = 0.72f; p.gridsY = 0.78f; p.gridsChaos = 0.50f;
            p.marblesRate = 1.25f; p.marblesJitter = 0.35f;

            p.channels[0].engine = 8; p.channels[0].decay = 0.45f; p.channels[0].drive = 0.3f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -8.0f;

            p.channels[1].engine = 3; p.channels[1].decay = 0.30f; p.channels[1].triggerSource = 4;
            p.channels[1].volumeDB = -8.5f; p.channels[1].pan = -0.2f;

            p.channels[2].engine = 6; p.channels[2].tune = 8.0f; p.channels[2].morph = 0.85f;
            p.channels[2].triggerSource = 5; p.channels[2].volumeDB = -12.0f; p.channels[2].pan = 0.35f;

            p.channels[3].engine = 11; p.channels[3].tune = -3.0f; p.channels[3].decay = 0.55f;
            p.channels[3].triggerSource = 6; p.channels[3].volumeDB = -13.5f; p.channels[3].aux1SendDB = -12.0f;

            presets.push_back(p);
        }

        // 14. IDM Chaos Theory (115 BPM)
        {
            PresetData p;
            p.name = "IDM Chaos Theory";
            p.category = "IDM";
            p.author = "MutableDSP";
            p.description = "Maximum Marbles jitter and bias modulation of Grids topographic map coordinates ($X, Y$), creating an evolving rhythm organism.";
            p.bpm = 115.0f;
            p.gridsX = 0.5f; p.gridsY = 0.5f; p.gridsChaos = 0.6f;
            p.gridsModMapX = 0.45f; p.gridsModMapY = 0.45f; p.gridsModChaos = 0.35f;
            p.marblesDejaVu = 0.4f; p.marblesBias = 0.6f;

            p.channels[0].engine = 1; p.channels[0].decay = 0.4f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;
            p.channels[1].engine = 9; p.channels[1].decay = 0.35f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.5f;
            p.channels[2].engine = 6; p.channels[2].tune = 6.0f; p.channels[2].triggerSource = 4; p.channels[2].volumeDB = -12.0f;
            p.channels[3].engine = 7; p.channels[3].decay = 0.25f; p.channels[3].triggerSource = 5; p.channels[3].volumeDB = -13.0f;

            presets.push_back(p);
        }

        // 15. IDM Granular Dust (108 BPM)
        {
            PresetData p;
            p.name = "IDM Granular Dust";
            p.category = "IDM";
            p.author = "MutableDSP";
            p.description = "Microscopic particle engine claps, diffuse modal bells and fluttering granular dust.";
            p.bpm = 108.0f;
            p.gridsX = 0.38f; p.gridsY = 0.28f; p.gridsChaos = 0.30f;
            p.delayTimeMs = 420.0f; p.delayFeedback = 0.6f; p.delayReturnDB = -13.0f;
            p.reverbDecay = 0.80f; p.reverbReturnDB = -15.0f;

            p.channels[0].engine = 0; p.channels[0].decay = 0.5f; p.channels[0].tone = 0.5f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -8.0f;
            p.channels[1].engine = 7; p.channels[1].decay = 0.45f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -10.5f; p.channels[1].aux1SendDB = -14.0f;
            p.channels[2].engine = 6; p.channels[2].tune = 11.0f; p.channels[2].decay = 0.35f; p.channels[2].triggerSource = 4; p.channels[2].volumeDB = -12.5f;
            p.channels[3].engine = 11; p.channels[3].tune = 16.0f; p.channels[3].decay = 0.7f; p.channels[3].triggerSource = 5; p.channels[3].volumeDB = -14.0f; p.channels[3].aux2SendDB = -12.0f;

            presets.push_back(p);
        }

        // 16. IDM Micro Clockwork (140 BPM)
        {
            PresetData p;
            p.name = "IDM Micro Clockwork";
            p.category = "IDM";
            p.author = "MutableDSP";
            p.description = "Rapid polyrhythmic clock multipliers, metallic bells and microtonal ride cymbals running like clockwork.";
            p.bpm = 140.0f;
            p.gridsX = 0.58f; p.gridsY = 0.68f; p.gridsChaos = 0.15f;
            p.marblesModel = 1; p.marblesRate = 1.5f;

            p.channels[0].engine = 8; p.channels[0].decay = 0.32f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -8.0f;
            p.channels[1].engine = 2; p.channels[1].decay = 0.25f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -9.0f;
            p.channels[2].engine = 4; p.channels[2].decay = 0.10f; p.channels[2].triggerSource = 4; p.channels[2].volumeDB = -13.5f;
            p.channels[3].engine = 10; p.channels[3].decay = 0.35f; p.channels[3].triggerSource = 5; p.channels[3].volumeDB = -15.0f;
            p.channels[4].engine = 11; p.channels[4].tune = 8.0f; p.channels[4].decay = 0.28f; p.channels[4].triggerSource = 6; p.channels[4].volumeDB = -13.0f;

            presets.push_back(p);
        }

        // =========================================================================
        // CATEGORY: HIP HOP & LO-FI (5 PRESETS)
        // =========================================================================

        // 17. Hip Hop Boom Bap 90s (92 BPM)
        {
            PresetData p;
            p.name = "Hip Hop Boom Bap 90s";
            p.category = "Hip Hop";
            p.author = "MutableDSP";
            p.description = "Fat, crunchy acoustic punch with warm vinyl crackle, loose organic swing, and tape saturated glue.";
            p.bpm = 92.0f;
            p.gridsX = 0.42f; p.gridsY = 0.35f; p.gridsChaos = 0.08f;
            p.gridsSwing = 0.22f;

            p.channels[0].engine = 0; p.channels[0].tune = 1.0f; p.channels[0].decay = 0.48f;
            p.channels[0].tone = 0.55f; p.channels[0].drive = 0.25f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 2; p.channels[1].tune = -1.0f; p.channels[1].decay = 0.38f;
            p.channels[1].timbre = 0.65f; p.channels[1].drive = 0.20f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.16f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.25f;

            p.channels[3].engine = 7; p.channels[3].decay = 0.30f; p.channels[3].triggerSource = 5;
            p.channels[3].volumeDB = -11.5f; p.channels[3].aux2SendDB = -18.0f;

            presets.push_back(p);
        }

        // 18. Hip Hop Dusty Lo-Fi Chill (85 BPM)
        {
            PresetData p;
            p.name = "Hip Hop Dusty Lo-Fi Chill";
            p.category = "Hip Hop";
            p.author = "MutableDSP";
            p.description = "Mellow rounded kick, soft sidechained snare, warm tape flutter and nostalgic chillhop atmosphere.";
            p.bpm = 85.0f;
            p.gridsX = 0.30f; p.gridsY = 0.22f; p.gridsChaos = 0.05f;
            p.gridsSwing = 0.25f;
            p.delayFeedback = 0.5f; p.delayTimeMs = 352.0f; p.delayReturnDB = -15.0f;
            p.reverbDecay = 0.75f; p.reverbReturnDB = -16.0f;

            p.channels[0].engine = 0; p.channels[0].tune = -2.0f; p.channels[0].decay = 0.42f;
            p.channels[0].tone = 0.35f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 2; p.channels[1].tune = 0.0f; p.channels[1].decay = 0.32f;
            p.channels[1].tone = 0.4f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.5f; p.channels[1].aux2SendDB = -15.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.12f; p.channels[2].tone = 0.45f;
            p.channels[2].triggerSource = 3; p.channels[2].volumeDB = -13.5f; p.channels[2].pan = -0.2f;

            p.channels[3].engine = 6; p.channels[3].tune = 5.0f; p.channels[3].decay = 0.28f;
            p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -13.0f; p.channels[3].aux1SendDB = -14.0f;

            presets.push_back(p);
        }

        // 19. Hip Hop Modern 808 Trap (140 BPM)
        {
            PresetData p;
            p.name = "Hip Hop Modern 808 Trap";
            p.category = "Hip Hop";
            p.author = "MutableDSP";
            p.description = "Booming sustained 808 sub kick with sharp piercing snare and rapid rolling triplet hi-hats.";
            p.bpm = 140.0f;
            p.gridsX = 0.70f; p.gridsY = 0.40f; p.gridsChaos = 0.12f;

            p.channels[0].engine = 0; p.channels[0].tune = -4.0f; p.channels[0].decay = 0.92f;
            p.channels[0].tone = 0.50f; p.channels[0].drive = 0.35f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.0f;

            p.channels[1].engine = 3; p.channels[1].tune = 2.0f; p.channels[1].decay = 0.40f;
            p.channels[1].timbre = 0.88f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.10f; p.channels[2].triggerSource = 3;
            p.channels[2].chokeGroup = 1; p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.22f;

            p.channels[3].engine = 5; p.channels[3].decay = 0.55f; p.channels[3].triggerSource = 6;
            p.channels[3].chokeGroup = 1; p.channels[3].volumeDB = -14.0f; p.channels[3].pan = 0.25f;

            p.channels[4].engine = 7; p.channels[4].decay = 0.26f; p.channels[4].triggerSource = 5;
            p.channels[4].volumeDB = -11.5f;

            presets.push_back(p);
        }

        // 20. Hip Hop Dilla Swing (88 BPM)
        {
            PresetData p;
            p.name = "Hip Hop Dilla Swing";
            p.category = "Hip Hop";
            p.author = "MutableDSP";
            p.description = "Heavy unquantized MPC micro-swing pocket, thick organic kick and warm saturated snare crack.";
            p.bpm = 88.0f;
            p.gridsX = 0.38f; p.gridsY = 0.44f; p.gridsChaos = 0.10f;
            p.gridsSwing = 0.32f;

            p.channels[0].engine = 0; p.channels[0].tune = 0.0f; p.channels[0].decay = 0.50f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 2; p.channels[1].tune = -2.0f; p.channels[1].decay = 0.44f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f; p.channels[1].aux2SendDB = -18.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.18f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.2f;

            p.channels[3].engine = 6; p.channels[3].tune = 4.0f; p.channels[3].decay = 0.32f;
            p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -12.5f;

            presets.push_back(p);
        }

        // 21. Hip Hop Neo Soul Pocket (90 BPM)
        {
            PresetData p;
            p.name = "Hip Hop Neo Soul Pocket";
            p.category = "Hip Hop";
            p.author = "MutableDSP";
            p.description = "Mellow woodblock rimshots, deep round kick, finger snaps, and silky smooth reverb wash.";
            p.bpm = 90.0f;
            p.gridsX = 0.32f; p.gridsY = 0.30f; p.gridsChaos = 0.06f;
            p.gridsSwing = 0.20f;
            p.reverbDecay = 0.65f; p.reverbReturnDB = -16.0f;

            p.channels[0].engine = 0; p.channels[0].decay = 0.45f; p.channels[0].tone = 0.45f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;
            p.channels[1].engine = 6; p.channels[1].tune = 7.0f; p.channels[1].decay = 0.22f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.5f;
            p.channels[2].engine = 4; p.channels[2].decay = 0.14f; p.channels[2].triggerSource = 3; p.channels[2].volumeDB = -13.5f; p.channels[2].pan = -0.25f;
            p.channels[3].engine = 7; p.channels[3].decay = 0.24f; p.channels[3].triggerSource = 5; p.channels[3].volumeDB = -11.5f; p.channels[3].aux2SendDB = -15.0f;

            presets.push_back(p);
        }

        // =========================================================================
        // CATEGORY: HOUSE & GARAGE (4 PRESETS)
        // =========================================================================

        // 22. House UK Garage 2-Step (132 BPM)
        {
            PresetData p;
            p.name = "House UK Garage 2-Step";
            p.category = "House";
            p.author = "MutableDSP";
            p.description = "Shuffling syncopated 2-step hi-hats, tight punchy kick on 1 and delayed on 2, snappy vocal clap.";
            p.bpm = 132.0f;
            p.gridsX = 0.55f; p.gridsY = 0.65f; p.gridsChaos = 0.14f;
            p.gridsSwing = 0.28f;

            p.channels[0].engine = 1; p.channels[0].tune = 1.0f; p.channels[0].decay = 0.42f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 7; p.channels[1].decay = 0.35f; p.channels[1].triggerSource = 2;
            p.channels[1].volumeDB = -8.5f; p.channels[1].aux2SendDB = -16.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.15f; p.channels[2].triggerSource = 3;
            p.channels[2].chokeGroup = 1; p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.25f;

            p.channels[3].engine = 5; p.channels[3].decay = 0.55f; p.channels[3].triggerSource = 6;
            p.channels[3].chokeGroup = 1; p.channels[3].volumeDB = -14.0f; p.channels[3].pan = 0.28f;

            presets.push_back(p);
        }

        // 23. House Deep Chicago Velvet (124 BPM)
        {
            PresetData p;
            p.name = "House Deep Chicago Velvet";
            p.category = "House";
            p.author = "MutableDSP";
            p.description = "Warm round 909 kick, classic rimshot, smooth closed hat grooves, and tape saturated bus.";
            p.bpm = 124.0f;
            p.gridsX = 0.50f; p.gridsY = 0.52f; p.gridsChaos = 0.08f;
            p.gridsSwing = 0.16f;

            p.channels[0].engine = 0; p.channels[0].tune = -1.0f; p.channels[0].decay = 0.60f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 2; p.channels[1].decay = 0.38f; p.channels[1].timbre = 0.70f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f; p.channels[1].aux2SendDB = -17.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.16f; p.channels[2].triggerSource = 3;
            p.channels[2].chokeGroup = 1; p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.22f;

            p.channels[3].engine = 5; p.channels[3].decay = 0.60f; p.channels[3].triggerSource = 6;
            p.channels[3].chokeGroup = 1; p.channels[3].volumeDB = -14.0f; p.channels[3].pan = 0.25f;

            p.channels[4].engine = 6; p.channels[4].tune = 4.0f; p.channels[4].decay = 0.35f;
            p.channels[4].triggerSource = 4; p.channels[4].volumeDB = -12.5f;

            presets.push_back(p);
        }

        // 24. House French Touch Filter (126 BPM)
        {
            PresetData p;
            p.name = "House French Touch Filter";
            p.category = "House";
            p.author = "MutableDSP";
            p.description = "Pumping compressed disco drum groove with high-pass and low-pass EQ curves, French electro snap.";
            p.bpm = 126.0f;
            p.gridsX = 0.75f; p.gridsY = 0.65f; p.gridsChaos = 0.10f;
            p.gridsSwing = 0.12f;

            p.channels[0].engine = 1; p.channels[0].tune = 0.0f; p.channels[0].decay = 0.50f;
            p.channels[0].drive = 0.30f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;
            p.channels[0].compThresh = -16.0f; p.channels[0].compRatio = 6.0f;

            p.channels[1].engine = 3; p.channels[1].decay = 0.40f; p.channels[1].timbre = 0.85f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f; p.channels[1].aux2SendDB = -16.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.15f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.0f;

            p.channels[3].engine = 7; p.channels[3].decay = 0.35f; p.channels[3].triggerSource = 5;
            p.channels[3].volumeDB = -11.0f;

            presets.push_back(p);
        }

        // 25. House Minimal Microhouse (125 BPM)
        {
            PresetData p;
            p.name = "House Minimal Microhouse";
            p.category = "House";
            p.author = "MutableDSP";
            p.description = "Subtle textured clicks, wooden clave percussions, and hypnotic minimalist house groove.";
            p.bpm = 125.0f;
            p.gridsX = 0.40f; p.gridsY = 0.48f; p.gridsChaos = 0.08f;
            p.gridsSwing = 0.14f;

            p.channels[0].engine = 0; p.channels[0].decay = 0.42f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;
            p.channels[1].engine = 2; p.channels[1].decay = 0.28f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -9.0f;
            p.channels[2].engine = 6; p.channels[2].tune = 8.0f; p.channels[2].decay = 0.20f; p.channels[2].triggerSource = 3; p.channels[2].volumeDB = -12.5f;
            p.channels[3].engine = 4; p.channels[3].decay = 0.10f; p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -13.5f;

            presets.push_back(p);
        }

        // =========================================================================
        // CATEGORY: DRUM & BASS & JUNGLE (4 PRESETS)
        // =========================================================================

        // 26. DnB Liquid Roller (174 BPM)
        {
            PresetData p;
            p.name = "DnB Liquid Roller";
            p.category = "Drum & Bass";
            p.author = "MutableDSP";
            p.description = "High-speed rolling 16th hats, punchy tight kick, bright snappy snare cutting through the mix.";
            p.bpm = 174.0f;
            p.gridsX = 0.68f; p.gridsY = 0.62f; p.gridsChaos = 0.12f;

            p.channels[0].engine = 1; p.channels[0].tune = 2.0f; p.channels[0].decay = 0.38f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 3; p.channels[1].tune = 3.0f; p.channels[1].decay = 0.32f;
            p.channels[1].timbre = 0.88f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -7.5f; p.channels[1].aux2SendDB = -18.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.10f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.22f;

            p.channels[3].engine = 5; p.channels[3].decay = 0.40f; p.channels[3].triggerSource = 6;
            p.channels[3].volumeDB = -14.0f; p.channels[3].pan = 0.25f;

            p.channels[4].engine = 6; p.channels[4].tune = 6.0f; p.channels[4].decay = 0.25f;
            p.channels[4].triggerSource = 4; p.channels[4].volumeDB = -13.0f;

            presets.push_back(p);
        }

        // 27. DnB Jungle Amen Vibe (168 BPM)
        {
            PresetData p;
            p.name = "DnB Jungle Amen Vibe";
            p.category = "Drum & Bass";
            p.author = "MutableDSP";
            p.description = "Classic breakbeat engine layout, pitched up snare, ghost notes, and crunchy tape distortion.";
            p.bpm = 168.0f;
            p.gridsX = 0.76f; p.gridsY = 0.70f; p.gridsChaos = 0.22f;

            p.channels[0].engine = 8; p.channels[0].tune = 1.0f; p.channels[0].decay = 0.35f;
            p.channels[0].drive = 0.30f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 9; p.channels[1].tune = 4.0f; p.channels[1].decay = 0.28f;
            p.channels[1].drive = 0.35f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -7.5f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.10f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.0f;

            p.channels[3].engine = 10; p.channels[3].decay = 0.35f; p.channels[3].triggerSource = 5;
            p.channels[3].volumeDB = -15.5f;

            presets.push_back(p);
        }

        // 28. DnB Neurofunk Tech Punch (172 BPM)
        {
            PresetData p;
            p.name = "DnB Neurofunk Tech Punch";
            p.category = "Drum & Bass";
            p.author = "MutableDSP";
            p.description = "Aggressive transient-designed kick and snare with precise surgical EQ and hard clipping.";
            p.bpm = 172.0f;
            p.gridsX = 0.82f; p.gridsY = 0.75f; p.gridsChaos = 0.18f;

            p.channels[0].engine = 1; p.channels[0].tune = 3.0f; p.channels[0].decay = 0.32f;
            p.channels[0].drive = 0.40f; p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;
            p.channels[0].compThresh = -16.0f; p.channels[0].compRatio = 6.0f;

            p.channels[1].engine = 3; p.channels[1].tune = 4.0f; p.channels[1].decay = 0.28f;
            p.channels[1].timbre = 0.92f; p.channels[1].drive = 0.35f; p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -7.5f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.08f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.5f;

            presets.push_back(p);
        }

        // 29. DnB Halftime Deep Space (86 BPM)
        {
            PresetData p;
            p.name = "DnB Halftime Deep Space";
            p.category = "Drum & Bass";
            p.author = "MutableDSP";
            p.description = "Heavy half-time groove with deep sub bass drops and cavernous reverb spaces.";
            p.bpm = 86.0f;
            p.gridsX = 0.48f; p.gridsY = 0.58f; p.gridsChaos = 0.14f;
            p.reverbDecay = 0.85f; p.reverbReturnDB = -14.0f;

            p.channels[0].engine = 0; p.channels[0].tune = -4.0f; p.channels[0].decay = 0.80f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.0f;

            p.channels[1].engine = 2; p.channels[1].tune = 2.0f; p.channels[1].decay = 0.45f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.0f; p.channels[1].aux2SendDB = -13.0f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.14f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.5f;

            p.channels[3].engine = 6; p.channels[3].tune = -5.0f; p.channels[3].decay = 0.60f;
            p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -12.0f; p.channels[3].aux2SendDB = -15.0f;

            presets.push_back(p);
        }

        // =========================================================================
        // CATEGORY: AMBIENT & ORGANIC (3 PRESETS)
        // =========================================================================

        // 30. Ambient Zen Garden (90 BPM)
        {
            PresetData p;
            p.name = "Ambient Zen Garden";
            p.category = "Ambient";
            p.author = "MutableDSP";
            p.description = "Meditative tuned modal bells, peaceful singing bowl resonances, and long shimmering reverbs.";
            p.bpm = 90.0f;
            p.gridsX = 0.30f; p.gridsY = 0.20f; p.gridsChaos = 0.15f;
            p.marblesModel = 0; p.marblesBias = 0.70f; p.marblesDejaVu = 0.40f;
            p.reverbDecay = 0.92f; p.reverbReturnDB = -12.0f;
            p.delayTimeMs = 500.0f; p.delayFeedback = 0.65f; p.delayReturnDB = -14.0f;

            p.channels[0].engine = 6; p.channels[0].tune = -12.0f; p.channels[0].decay = 0.80f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -8.5f; p.channels[0].aux2SendDB = -14.0f;

            p.channels[1].engine = 11; p.channels[1].tune = 0.0f; p.channels[1].decay = 0.88f;
            p.channels[1].triggerSource = 4; p.channels[1].volumeDB = -11.0f; p.channels[1].aux2SendDB = -12.0f;

            p.channels[2].engine = 7; p.channels[2].decay = 0.60f; p.channels[2].triggerSource = 5;
            p.channels[2].volumeDB = -13.0f; p.channels[2].aux2SendDB = -15.0f;

            p.channels[3].engine = 6; p.channels[3].tune = 7.0f; p.channels[3].decay = 0.75f;
            p.channels[3].triggerSource = 6; p.channels[3].volumeDB = -12.0f; p.channels[3].aux1SendDB = -13.0f;

            presets.push_back(p);
        }

        // 31. Ambient Ethereal Diffusion (75 BPM)
        {
            PresetData p;
            p.name = "Ambient Ethereal Diffusion";
            p.category = "Ambient";
            p.author = "MutableDSP";
            p.description = "Slow floating rhythmic textures, generative random Marbles pulses, and atmospheric delays.";
            p.bpm = 75.0f;
            p.gridsX = 0.20f; p.gridsY = 0.18f; p.gridsChaos = 0.20f;
            p.marblesRate = 0.75f; p.marblesDejaVu = 0.30f;
            p.reverbDecay = 0.95f; p.reverbReturnDB = -12.0f;

            p.channels[0].engine = 6; p.channels[0].tune = -7.0f; p.channels[0].decay = 0.85f;
            p.channels[0].triggerSource = 4; p.channels[0].volumeDB = -9.0f; p.channels[0].aux2SendDB = -12.0f;

            p.channels[1].engine = 11; p.channels[1].tune = 5.0f; p.channels[1].decay = 0.90f;
            p.channels[1].triggerSource = 5; p.channels[1].volumeDB = -11.5f; p.channels[1].aux2SendDB = -12.0f;

            p.channels[2].engine = 7; p.channels[2].decay = 0.70f; p.channels[2].triggerSource = 6;
            p.channels[2].volumeDB = -13.5f; p.channels[2].aux1SendDB = -13.0f;

            presets.push_back(p);
        }

        // 32. Tribal Afrobeat Topology (118 BPM)
        {
            PresetData p;
            p.name = "Tribal Afrobeat Topology";
            p.category = "World";
            p.author = "MutableDSP";
            p.description = "Polyrhythmic modal toms, resonant woodblocks, organic shaker hats, and infectious syncopation.";
            p.bpm = 118.0f;
            p.gridsX = 0.65f; p.gridsY = 0.58f; p.gridsChaos = 0.16f;
            p.gridsSwing = 0.24f;

            p.channels[0].engine = 0; p.channels[0].tune = 0.0f; p.channels[0].decay = 0.52f;
            p.channels[0].triggerSource = 1; p.channels[0].volumeDB = -7.5f;

            p.channels[1].engine = 6; p.channels[1].tune = -2.0f; p.channels[1].decay = 0.40f;
            p.channels[1].triggerSource = 2; p.channels[1].volumeDB = -8.5f;

            p.channels[2].engine = 4; p.channels[2].decay = 0.14f; p.channels[2].triggerSource = 3;
            p.channels[2].volumeDB = -13.0f; p.channels[2].pan = -0.3f;

            p.channels[3].engine = 6; p.channels[3].tune = 5.0f; p.channels[3].decay = 0.35f;
            p.channels[3].triggerSource = 4; p.channels[3].volumeDB = -12.0f; p.channels[3].pan = 0.35f;

            p.channels[4].engine = 7; p.channels[4].decay = 0.28f; p.channels[4].triggerSource = 5;
            p.channels[4].volumeDB = -11.5f;

            presets.push_back(p);
        }

        return presets;
    }
};

} // namespace MutableDrums
